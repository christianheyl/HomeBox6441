//------------------------------------------------------------------------------
// Copyright (c) 2011 Atheros Communications Inc.
// All rights reserved.
//
// $ATH_LICENSE_HOSTSDK0_C$
//------------------------------------------------------------------------------


#include "precomp.h"
#if defined(HIF_USB)
#include "hif_usb_kmdf.h"
#include "hif_usb_internal.h"
#include "wdf.h"
#include "ath_hif_usb.h"

#define ATH_MODULE_NAME hif_usb_kmdf
#include <a_debug.h>
#include "hif_usb_internal.h"

#define HIF_USB_KMDF_DEBUG   ATH_DEBUG_MAKE_MODULE_MASK(0)

#if defined(DEBUG)
static ATH_DEBUG_MASK_DESCRIPTION g_HIFDebugDescription[] = {
    {HIF_USB_KMDF_DEBUG,"hif_usb_kmdf"},
};

ATH_DEBUG_INSTANTIATE_MODULE_VAR(hif_usb_kmdf,
                                 "hif_usb_kmdf",
                                 "USB Host KMDF Interface",
                                 ATH_DEBUG_MASK_DEFAULTS | ATH_DEBUG_INFO | HIF_USB_KMDF_DEBUG,
                                 ATH_DEBUG_DESCRIPTION_COUNT(g_HIFDebugDescription),
                                 g_HIFDebugDescription);
#endif


static NTSTATUS initKMDF(    
    __in struct _DRIVER_OBJECT *DriverObject,
    __in PUNICODE_STRING RegistryPath )
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS  ntStatus;
    
    WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
    
    //
    // Set WdfDriverInitNoDispatchOverride flag to tell the framework
    // not to provide dispatch routines for the driver. In other words,
    // the framework must not intercept IRPs that the I/O manager has
    // directed to the driver. In this case, it will be handled by NDIS
    // port driver.
    //
    config.DriverInitFlags |= WdfDriverInitNoDispatchOverride;

    ntStatus = WdfDriverCreate(DriverObject,
                               RegistryPath,
                               WDF_NO_OBJECT_ATTRIBUTES,
                               &config,
                               WDF_NO_HANDLE);
    if (!NT_SUCCESS(ntStatus)){
        printk("WdfDriverCreate failed\n");
        return NDIS_STATUS_FAILURE;
    }
    else
    {
        printk("WdfDriverCreate OK...\n");
        return ntStatus;
    }        
}

NTSTATUS initUsbKMDF(    
    __in struct _DRIVER_OBJECT *DriverObject,
    __in PUNICODE_STRING RegistryPath )
{ 
    int i;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    for (i = 0; i < 10; i++)
    {
        Status = initKMDF(DriverObject, RegistryPath);
        if (NT_SUCCESS(Status))
        {
            break;
        }
        NdisMSleep(1000*1000);
    }
    
    return Status;
}

NTSTATUS HIFUsbCreateWdfDevice(PADAPTER pAdapter)
{
    WDF_OBJECT_ATTRIBUTES    attributes;
    NDIS_STATUS              ndisStatus = NDIS_STATUS_SUCCESS;
    ULONG                    nameLength;

    NdisMGetDeviceProperty(pAdapter->MiniportAdapterHandle,
                           &pAdapter->Pdo,
                           &pAdapter->Fdo,
                           &pAdapter->NextDeviceObject,
                           NULL,
                           NULL
                          );

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WDF_DEVICE_INFO);
    
    ndisStatus = IoGetDeviceProperty(pAdapter->Pdo,
                               DevicePropertyDeviceDescription,
                               NIC_ADAPTER_NAME_SIZE,
                               pAdapter->AdapterDesc,
                               &nameLength);
    if (!NT_SUCCESS (ndisStatus))
    {
        printk("IoGetDeviceProperty failed  status:(0x%x)", ndisStatus);
        return -ENOMEM;
    }
    
    ndisStatus = WdfDeviceMiniportCreate(WdfGetDriver(),
                                       &attributes,
                                       pAdapter->Fdo,
                                       pAdapter->NextDeviceObject,
                                       pAdapter->Pdo,
                                       &pAdapter->WdfDevice);    
    if (!NT_SUCCESS (ndisStatus))
    {
        printk("WdfDeviceMiniportCreate failed  status:(0x%x)", ndisStatus);
        return -ENOMEM;
    }
    return ndisStatus;
}

NTSTATUS KmdfUsbSelectInterfaces(WDFDEVICE Device, HIF_KMDF_USB_CB *pUsbCb)
{
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    NTSTATUS                            status;
    UCHAR numUsbInterfaces;

    //
    // The device has only 1 interface.
    //
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);

    status = WdfUsbTargetDeviceSelectConfig(
                pUsbCb->WdfUsbTargetDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);

    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    numUsbInterfaces = WdfUsbTargetDeviceGetNumInterfaces(pUsbCb->WdfUsbTargetDevice);

    if (numUsbInterfaces > 0) {
        pUsbCb->UsbInterface =
            configParams.Types.SingleInterface.ConfiguredUsbInterface;

        pUsbCb->NumberConfiguredPipes =
            configParams.Types.SingleInterface.NumberConfiguredPipes;
    }

    return status;
}

NTSTATUS KmdfUsbConfigureDevice(WDFDEVICE Device, HIF_KMDF_USB_CB *pUsbCb)
{
    USHORT                        size = 0;
    NTSTATUS                      status = NDIS_STATUS_SUCCESS;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY   memory;

    //
    // initialize the variables
    //
    configurationDescriptor = NULL;

    //
    // Read the first configuration descriptor
    // This requires two steps:
    // 1. Ask the WDFUSBDEVICE how big it is
    // 2. Allocate it and get it from the WDFUSBDEVICE
    //
    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                pUsbCb->WdfUsbTargetDevice, WDF_NO_HANDLE, &size);

    if (status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    //
    // Create a memory object and specify usbdevice as the parent so that
    // it will be freed automatically.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    attributes.ParentObject = pUsbCb->WdfUsbTargetDevice;

    status = WdfMemoryCreate(&attributes,
                             NonPagedPool,
                             'athr',
                             size,
                             &memory,
                             &configurationDescriptor);
                             
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                pUsbCb->WdfUsbTargetDevice, configurationDescriptor, &size);
    
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pUsbCb->UsbConfigurationDescriptor = configurationDescriptor;

    status = KmdfUsbSelectInterfaces(Device, pUsbCb);

    return status;   
}

void initRxRequestList(hif_handle_t hif_hdl, 
                       WDFCOLLECTION freeList,
                       int nItems)
{
    struct ath_hif_pci_softc *sc = hif_hdl;
    struct ol_ath_softc_net80211 *scn = sc->scn;
    PADAPTER  pAdapter = scn->sc_osdev->pAdapter;
    int i;
    UsbRxRequestItem *pItem;
    WDFOBJECT rxObj;
    WDF_OBJECT_ATTRIBUTES objAttributes;
    NTSTATUS status; 
    PUCHAR pMemory = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objAttributes, UsbRxRequestItem);
                                                           
    for(i = 0; i < nItems; i++)
    {
        rxObj = NULL;
        
        status = WdfObjectCreate(&objAttributes, &rxObj);
        if (!NT_SUCCESS(status)) {
            continue;
        }
                       
        pItem = (UsbRxRequestItem *)GetUsbRxRequestItem(rxObj);
        ASSERT(pItem != NULL);

        pItem->pAdapter = pAdapter;
        pItem->sc = sc;
                                  
        status = WdfRequestCreate(&objAttributes, 
                                  NULL, 
                                  &pItem->UsbInRequest);
        if (!NT_SUCCESS(status)) {
            goto no_resource;
        }

        NdisAllocateMemoryWithTag(
            (PVOID *)&pMemory,
            HIF_USB_RX_BUFFER_SIZE,//HIF_USB_RX_STREAM_BUFFER_SIZE,
            'RXBF');

        if (pMemory != NULL) {
            pItem->rxwbuf = pMemory;
        }
        else {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: Can't allocate rx buffer for stream mode\n", __FUNCTION__));
            goto no_resource;
        }

        {
            WDF_OBJECT_ATTRIBUTES  attributes;         
            
            WDF_OBJECT_ATTRIBUTES_INIT(&attributes);            
            status = WdfMemoryCreatePreallocated(
                                                 &attributes,
                                                 pItem->rxwbuf,
                                                 HIF_USB_RX_BUFFER_SIZE,//HIF_USB_RX_STREAM_BUFFER_SIZE,
                                                 &pItem->UsbRequestBuffer);
        }

        status = WdfCollectionAdd(freeList, rxObj);
        if (!NT_SUCCESS(status)) {
            goto no_resource;
        }

        continue;

no_resource:
        ASSERT(rxObj != NULL);
        ASSERT(status == STATUS_INSUFFICIENT_RESOURCES);
        WdfObjectDelete(rxObj);
        continue;
    }
}

void initRequestFreeList(hif_handle_t hif_hdl, 
                         WDFCOLLECTION freeList,
                         int nItems,
                         int length,
                         UCHAR PipeIndex)
{
    struct ath_hif_pci_softc *sc = hif_hdl;
    struct ol_ath_softc_net80211 *scn = sc->scn;
    PADAPTER  pAdapter = scn->sc_osdev->pAdapter;
    int i;
    UsbOutRequestItem *pItem;
    WDFOBJECT TxObj;
    WDF_OBJECT_ATTRIBUTES objAttributes;
    NTSTATUS status;

    WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objAttributes, UsbOutRequestItem);

    for (i = 0; i < nItems; i++)
    {

        PUCHAR pMemory = NULL;

        WDF_OBJECT_ATTRIBUTES  attributes;         
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        TxObj = NULL;

        status = WdfObjectCreate(&objAttributes, &TxObj);
        if (!NT_SUCCESS(status)) {
            continue;
        }
                       
        pItem = (UsbOutRequestItem *)GetUsbOutRequestItem(TxObj);
        ASSERT(pItem != NULL);

        pItem->pAdapter = pAdapter;
        pItem->sc = sc;
        pItem->IsUsed = 0;
        pItem->offset = 0;
        pItem->txwbuf = NULL;
        pItem->tx_adf_buf = NULL;
        pItem->PipeIndex = PipeIndex;

        status = WdfRequestCreate(&objAttributes, 
                                  NULL, 
                                  &pItem->UsbOutRequest);
        if (!NT_SUCCESS(status)) {
            goto no_resource;
        }
                                     
        WdfMemoryCreate(&objAttributes,
                        NonPagedPool,
                        'ATHT',
                        sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER), 
                        &pItem->UsbUrbBuffer,
                        NULL); 
        NdisAllocateMemoryWithTag(
            (PVOID *)&pMemory,
            length,//HIF_USB_TX_STREAM_BUFFER_SIZE,
            'TXBF');

        pItem->txwbuf = pMemory;

        status = WdfMemoryCreatePreallocated(
                                             &attributes,
                                             pItem->txwbuf,
                                             length,//HIF_USB_TX_STREAM_BUFFER_SIZE,
                                             &pItem->UsbRequestBuffer);

        status = WdfCollectionAdd(freeList, TxObj);
        if (!NT_SUCCESS(status)) {
            goto no_resource;
        }
        continue;
        
no_resource:
        ASSERT(TxObj != NULL);
        ASSERT(status == STATUS_INSUFFICIENT_RESOURCES);
        WdfObjectDelete(TxObj);
        continue;                                  
    }

}

void initZeroRequestFreeList(hif_handle_t hif_hdl,
                         WDFCOLLECTION freeList,
                         int nItems,
                         int length,
                         UCHAR PipeIndex)
{
    struct ath_hif_pci_softc *sc = hif_hdl;
    struct ol_ath_softc_net80211 *scn = sc->scn;
    PADAPTER  pAdapter = scn->sc_osdev->pAdapter;
    int i;
    UsbZeroOutRequestItem *pItem;
    WDFOBJECT TxObj;
    WDF_OBJECT_ATTRIBUTES objAttributes;
    NTSTATUS status;
    PUCHAR pMemory = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objAttributes, UsbZeroOutRequestItem);

    for (i = 0; i < nItems; i++)
    {
        WDF_OBJECT_ATTRIBUTES  attributes;         
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        TxObj = NULL;

        status = WdfObjectCreate(&objAttributes, &TxObj);
        if (!NT_SUCCESS(status)) {
            continue;
        }
                       
        pItem = (UsbZeroOutRequestItem *)GetUsbZeroOutRequestItem(TxObj);
        ASSERT(pItem != NULL);

        pItem->pAdapter = pAdapter;
        pItem->sc = sc;
        pItem->IsUsed = 0;
        pItem->offset = 0;
        pItem->txwbuf = NULL;
        pItem->PipeIndex = PipeIndex;

        status = WdfRequestCreate(&objAttributes, 
                                  NULL, 
                                  &pItem->UsbOutRequest);
        if (!NT_SUCCESS(status)) {
            goto no_resource;
        }
                                     
        WdfMemoryCreate(&objAttributes,
                        NonPagedPool,
                        'ATHT',
                        sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER), 
                        &pItem->UsbUrbBuffer,
                        NULL);
      

        NdisAllocateMemoryWithTag(
                (PVOID *)&pMemory,
                length,
                'TXBF');

        pItem->txwbuf = pMemory;
            
        status = WdfMemoryCreatePreallocated(
                                            &attributes,
                                            pItem->txwbuf,
                                            length,
                                            &pItem->UsbRequestBuffer);

        status = WdfCollectionAdd(freeList, TxObj);
        if (!NT_SUCCESS(status)) {
            goto no_resource;
        }

        continue;
        
no_resource:
        ASSERT(TxObj != NULL);
        ASSERT(status == STATUS_INSUFFICIENT_RESOURCES);
        WdfObjectDelete(TxObj);
        continue;                                  
    }
}

NTSTATUS KmdfUSbSetupPipeResources(HIF_DEVICE_USB *device)
{
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    struct ath_hif_pci_softc *sc = device->sc;
    struct ol_ath_softc_net80211 *scn = sc->scn;
    PADAPTER  pAdapter = scn->sc_osdev->pAdapter;
    NTSTATUS status;
    WDFUSBPIPE pipe;
    int i;
    WDF_USB_PIPE_INFORMATION        UsbRxPipeInfo;

    /* Initialize resources */
    NdisAllocateSpinLock(&pUsbCb->UsbRxListLock);
    status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES, &pUsbCb->UsbRxFreeList);

    if (!NT_SUCCESS(status)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s WdfCollectionCreate UsbRxFreeList fail\n", __FUNCTION__));
        return STATUS_UNSUCCESSFUL;
    }

    status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES, &pUsbCb->UsbRxUsedList);
    if (!NT_SUCCESS(status)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s WdfCollectionCreate UsbRxUsedList fail\n", __FUNCTION__));
        return STATUS_UNSUCCESSFUL;
    }

	initRxRequestList(device->sc, pUsbCb->UsbRxFreeList,
            ATH_USB_RX_CELL_NUMBER);

    pipe = WdfUsbInterfaceGetConfiguredPipe(pUsbCb->UsbInterface,
    usb_hif_get_usb_epnum(HIF_RX_DATA_PIPE) - 1, NULL);
    WdfUsbTargetPipeGetInformation(pipe, &UsbRxPipeInfo); 
    if (UsbRxPipeInfo.MaximumPacketSize == 64) {
        pUsbCb->bFullSpeed = TRUE;
    }
    
    if (pUsbCb->bFullSpeed) {
        pipe = WdfUsbInterfaceGetConfiguredPipe(pUsbCb->UsbInterface,
                    usb_hif_get_usb_epnum(HIF_RX_INT_PIPE) - 1, NULL);
    } else {
        pipe = WdfUsbInterfaceGetConfiguredPipe(pUsbCb->UsbInterface,
                    usb_hif_get_usb_epnum(HIF_RX_DATA_PIPE) - 1, NULL);
    }

    WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);
    
    pUsbCb->RxPipe = pipe;

    /* Initialize Tx pipe resources */
    for (i = 0; i < HIF_USB_TX_PIPE_NUM; i++) {
        NdisAllocateSpinLock(&(pUsbCb->USB_TX_PIPE[i].UsbTxListLock));
        NdisAllocateSpinLock(&(pUsbCb->USB_TX_PIPE[i].UsbTxSendLock));

        status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES,
                    &(pUsbCb->USB_TX_PIPE[i].UsbTxFreeList));
        if (!NT_SUCCESS(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s WdfCollectionCreate UsbTxFreeList fail\n", __FUNCTION__));
            return STATUS_UNSUCCESSFUL;
        }

        status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES,
                    &pUsbCb->USB_TX_PIPE[i].UsbTxUsedList);
        if (!NT_SUCCESS(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s WdfCollectionCreate UsbTxUsedList fail\n", __FUNCTION__));
            return STATUS_UNSUCCESSFUL;
        }

        status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES,
                    &(pUsbCb->USB_TX_PIPE[i].UsbTxZeroFreeList));
        if (!NT_SUCCESS(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s WdfCollectionCreate UsbTxZeroFreeList fail\n", __FUNCTION__));
            return STATUS_UNSUCCESSFUL;
        }

        status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES,
                    &pUsbCb->USB_TX_PIPE[i].UsbTxZeroUsedList);
        if (!NT_SUCCESS(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s WdfCollectionCreate UsbTxZeroUsedList fail\n", __FUNCTION__));
            return STATUS_UNSUCCESSFUL;
        }

        pUsbCb->USB_TX_PIPE[i].usbTxCntPerSec = 0;
        pUsbCb->USB_TX_PIPE[i].usbTxPktCnt = 0;
        pUsbCb->USB_TX_PIPE[i].TxPktQueued = FALSE;
        initRequestFreeList(device->sc, pUsbCb->USB_TX_PIPE[i].UsbTxFreeList,
                ATH_USB_TX_CELL_NUMBER, HIF_USB_TX_BUFFER_SIZE, i);
        initZeroRequestFreeList(device->sc, pUsbCb->USB_TX_PIPE[i].UsbTxZeroFreeList,
                ATH_USB_TXZERO_CELL_NUMBER, HIF_USB_TXZERO_BUFFER_SIZE, i);
    }

    for (i = 0; i < HIF_USB_PIPE_MAX; i++)
    {
        pUsbCb->USBRestPipe[i] = (PUsbPipeResetInfo)NdisAllocateMemoryWithTagPriority(pAdapter->MiniportAdapterHandle, sizeof(UsbPipeResetInfo), 'TSRU', HighPoolPriority);
        if (pUsbCb->USBRestPipe[i]!= NULL)
        {
            pUsbCb->USBRestPipe[i]->device = device;
            pUsbCb->USBRestPipe[i]->epId  = i;
        }
        else
        {
            for (i = 0; i < HIF_USB_PIPE_MAX; i++)
            {
                if(pUsbCb->USBRestPipe[i] != NULL)
                {
                    NdisFreeMemory(pUsbCb->USBRestPipe[i], 0, 0);
                }       
            }
            return STATUS_UNSUCCESSFUL;
        }
    }
    return STATUS_SUCCESS;
}

static void freeOutRequests(HIF_KMDF_USB_CB *pUsbCb, 
                               WDFCOLLECTION UsbUsedList,
                               WDFCOLLECTION UsbFreeList,
                               PNDIS_SPIN_LOCK lock,
                               IN UCHAR isForTxQ)
{
    int nCmdRequest;
    int i;
    WDFOBJECT cmdObj;
    UsbOutRequestItem *pItem;
    BOOLEAN cancelRes;
    HIF_DEVICE_USB              *device = NULL;
    
    NdisAcquireSpinLock(lock);
    if (UsbUsedList == NULL) {
        goto done;
    }

    ASSERT( WdfCollectionGetCount(UsbUsedList) == 0 );
    nCmdRequest = WdfCollectionGetCount(UsbFreeList);

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Free nCmdRequest %d\n", nCmdRequest));

    if (nCmdRequest == 0) {
        goto done;
    }     

    for (i = 0; i < nCmdRequest; i++)
    {     
        cmdObj = WdfCollectionGetFirstItem(UsbFreeList);
        WdfCollectionRemove(UsbFreeList, cmdObj);
        
        pItem = (UsbOutRequestItem *)GetUsbOutRequestItem(cmdObj);

        if (pItem->tx_adf_buf != NULL) {
            device = pItem->sc->UsbBusInterface;
            device->htcCallbacks.txCompletionHandler(device->htcCallbacks.Context,
                                                     pItem->tx_adf_buf, pItem->transfer_id);
        }
        WdfObjectDelete(pItem->UsbOutRequest);
        
        if (pItem->UsbRequestBuffer != NULL)
        {
            NdisFreeMemory(pItem->txwbuf, HIF_USB_TX_BUFFER_SIZE, 0);
            WdfObjectDelete(pItem->UsbRequestBuffer);
        }
        
        WdfObjectDelete(cmdObj);
    }        
    
done:    
    NdisReleaseSpinLock(lock);  
}

static void freeZeroOutRequests(HIF_KMDF_USB_CB *pUsbCb, 
                               WDFCOLLECTION UsbUsedList,
                               WDFCOLLECTION UsbFreeList,
                               PNDIS_SPIN_LOCK lock,
                               IN UCHAR isForTxQ)
{
    int nCmdRequest;
    int i;
    WDFOBJECT cmdObj;
    UsbZeroOutRequestItem *pItem;
    BOOLEAN cancelRes;
    
    NdisAcquireSpinLock(lock);
    if (UsbUsedList == NULL) {
        goto done;
    }

    ASSERT( WdfCollectionGetCount(UsbUsedList) == 0 );
    nCmdRequest = WdfCollectionGetCount(UsbFreeList);

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Free nCmdRequest %d\n", nCmdRequest));

    if (nCmdRequest == 0) {
        goto done;
    }     

    for (i = 0; i < nCmdRequest; i++)
    {     
        cmdObj = WdfCollectionGetFirstItem(UsbFreeList);
        WdfCollectionRemove(UsbFreeList, cmdObj);
        
        pItem = (UsbZeroOutRequestItem *)GetUsbZeroOutRequestItem(cmdObj);
    
        WdfObjectDelete(pItem->UsbOutRequest);
        
        if (pItem->UsbRequestBuffer != NULL)
        {
            NdisFreeMemory(pItem->txwbuf, HIF_USB_TXZERO_BUFFER_SIZE, 0);
            WdfObjectDelete(pItem->UsbRequestBuffer);
        }
        
        WdfObjectDelete(cmdObj);
    }        
    
done:    
    NdisReleaseSpinLock(lock);  
}

static void freeInRequests(HIF_KMDF_USB_CB *pUsbCb, 
                               WDFCOLLECTION UsbUsedList,
                               WDFCOLLECTION UsbFreeList,
                               PNDIS_SPIN_LOCK lock)
{
    int nRequest;
    int i;
    WDFOBJECT rxObj;
    UsbRxRequestItem *pItem;
    BOOLEAN cancelRes;
    
    NdisAcquireSpinLock(lock);
    if(UsbUsedList == NULL)
    {
        goto done;
    }        
    ASSERT(WdfCollectionGetCount(UsbUsedList) == 0);
    nRequest = WdfCollectionGetCount(UsbFreeList);

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Free nRequest %d\n", nRequest));    
    if(nRequest == 0)
    {
        goto done;
    }     
    
    for(i = 0; i < nRequest; i++)
    {     
        rxObj = WdfCollectionGetFirstItem(UsbFreeList);
        WdfCollectionRemove(UsbFreeList, rxObj);
        
        pItem = (UsbRxRequestItem *)GetUsbRxRequestItem(rxObj);
    
        WdfObjectDelete(pItem->UsbInRequest);
        
        if (pItem->UsbRequestBuffer != NULL)
        {
            NdisFreeMemory(pItem->rxwbuf, HIF_USB_RX_BUFFER_SIZE, 0);
            WdfObjectDelete(pItem->UsbRequestBuffer);
        }
     
        WdfObjectDelete(rxObj);
    }        
    
done:    
    NdisReleaseSpinLock(lock);  
}

NTSTATUS KmdfUSbReturnPipeResources(HIF_DEVICE_USB *device)
{
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    NTSTATUS status;
    WDFUSBPIPE pipe;
    int i;

    for (i = 0; i < HIF_USB_TX_PIPE_NUM; i++) {
        freeOutRequests(pUsbCb, 
                        pUsbCb->USB_TX_PIPE[i].UsbTxUsedList, 
                        pUsbCb->USB_TX_PIPE[i].UsbTxFreeList,
                        &(pUsbCb->USB_TX_PIPE[i].UsbTxListLock),
                        1);
        freeZeroOutRequests(pUsbCb, 
                        pUsbCb->USB_TX_PIPE[i].UsbTxZeroUsedList, 
                        pUsbCb->USB_TX_PIPE[i].UsbTxZeroFreeList,
                        &(pUsbCb->USB_TX_PIPE[i].UsbTxListLock),
                        1);  
        NdisFreeSpinLock(&(pUsbCb->USB_TX_PIPE[i].UsbTxListLock));
        NdisFreeSpinLock(&(pUsbCb->USB_TX_PIPE[i].UsbTxSendLock));
    }

    freeInRequests(pUsbCb, 
                    pUsbCb->UsbRxUsedList, 
                    pUsbCb->UsbRxFreeList,
                    &pUsbCb->UsbRxListLock);

    NdisFreeSpinLock(&pUsbCb->UsbRxListLock);

    for (i = 0; i < HIF_USB_PIPE_MAX; i++)
    {
        if(pUsbCb->USBRestPipe[i] != NULL)
        {
            NdisFreeMemory(pUsbCb->USBRestPipe[i], 0, 0);
        }
    }

    if (pUsbCb->WdfUsbTargetDevice != NULL)
        WdfObjectDelete(pUsbCb->WdfUsbTargetDevice);

    return A_OK;
}


void
KmdfUsbSchedulePipeReset(
    HIF_DEVICE_USB *device,
    PUsbPipeResetInfo        pUsbPipeResetInfo    
)
{
    struct ath_hif_pci_softc *sc = device->sc;
    struct ol_ath_softc_net80211 *scn = sc->scn;
    PADAPTER  pAdapter = scn->sc_osdev->pAdapter;
    NDIS_HANDLE         NdisIoWorkItemHandle = NULL;
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    
    MP_INTERLOCKED_SET_BIT(pUsbCb->UsbPipeResetFlag, pUsbPipeResetInfo->epId);
    
    NdisIoWorkItemHandle = NdisAllocateIoWorkItem(pAdapter->MiniportAdapterHandle);  
    if (NdisIoWorkItemHandle && pUsbPipeResetInfo) {
        NdisQueueIoWorkItem(NdisIoWorkItemHandle, KmdfUsbPipeResetRoutine, pUsbPipeResetInfo);
    }
    else {
        if (NdisIoWorkItemHandle) {
            NdisFreeIoWorkItem(NdisIoWorkItemHandle);
        }
    }
}

NTSTATUS
KmdfUsbPipeAbortSync(
    HIF_DEVICE_USB *device,
    LONG            epId
)
{
#define USB_PIPEABORT_DELAY_TIME  1000
#define USB_PIPEABORT_TRY_CNT     5
    
    WDFUSBPIPE  hPipe = NULL;
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    int         i = 0;
    
    if (device->bUSBDeveiceAttached == FALSE) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s_%d: USB device detached !, epId = %d\n", __FUNCTION__, __LINE__, epId));
        return STATUS_UNSUCCESSFUL;
    }
    
    if (epId < HIF_RX_CTRL_PIPE) {
        hPipe =  device->usbCb.USB_TX_PIPE[epId].UsbTxPipe;
    } else {
        hPipe =  device->usbCb.RxPipe;
    }
    
    if (hPipe) {
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: Abort start, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
        do {
            ntStatus = WdfUsbTargetPipeAbortSynchronously(hPipe, NULL, NULL);
            ++i;
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: Abort return, epId = %d, try_cnt = %d, ntStatus = 0x%x\n",  __FUNCTION__, __LINE__, epId, i, ntStatus));        
            if (NT_SUCCESS(ntStatus)) {
                break;
            } else if (ntStatus == STATUS_NO_SUCH_DEVICE) {
                AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: USB device detached !, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
                break;
            } else {
                NdisMSleep(USB_PIPEABORT_DELAY_TIME);
            }
        } while (i < USB_PIPEABORT_TRY_CNT);
    }
    
    return ntStatus;
    
#undef USB_PIPEABORT_TRY_CNT
#undef USB_PIPEABORT_DELAY_TIME
}

NTSTATUS
KmdfUsbPipeResetSync(
    HIF_DEVICE_USB *device,
    LONG            epId
)
{
#define USB_PIPERESET_DELAY_TIME  1000
#define USB_PIPERESET_TRY_CNT     5
    
    WDFUSBPIPE  hPipe = NULL;
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    int         i = 0;
    
    if (device->bUSBDeveiceAttached == FALSE) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s_%d: USB device detached !, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
        return STATUS_UNSUCCESSFUL;
    }

    if (epId < HIF_RX_CTRL_PIPE) {
        hPipe =  device->usbCb.USB_TX_PIPE[epId].UsbTxPipe;
    } else {
        hPipe =  device->usbCb.RxPipe;
    }
    
    if (hPipe) {
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: Pipe reset start, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
        do {
            ntStatus = WdfUsbTargetPipeResetSynchronously(hPipe, NULL, NULL);
            ++i;
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: Pipe reset return, epId = %d, try_cnt = %d, ntStatus = 0x%x\n",  __FUNCTION__, __LINE__, epId, i, ntStatus));        
            if (NT_SUCCESS(ntStatus)) {
                break;
            } else if (ntStatus == STATUS_NO_SUCH_DEVICE) {
               AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: USB device detached !, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
                break;
            } else {
                NdisMSleep(USB_PIPERESET_DELAY_TIME);
            }
        } while (i < USB_PIPERESET_TRY_CNT);
    }
    
    return ntStatus;
    
#undef USB_PIPERESET_TRY_CNT
#undef USB_PIPERESET_DELAY_TIME
}

NTSTATUS
KmdfUsbPipeSubmitUrb(
    HIF_DEVICE_USB *device,
    LONG            epId
)
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    WDFUSBPIPE  hPipe = NULL;
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    int i, j;
    WDFOBJECT   freeObj = NULL;
    
    if (device->bUSBDeveiceAttached == FALSE) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s_%d: USB device detached !, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
        return STATUS_UNSUCCESSFUL;
    }
    
    if (epId < HIF_RX_CTRL_PIPE) {
        hPipe =  device->usbCb.USB_TX_PIPE[epId].UsbTxPipe;
    } else {
        hPipe =  device->usbCb.RxPipe;
    }

     if (hPipe) {
       if (epId < HIF_RX_CTRL_PIPE) {
            if (device->bUSBDeveiceAttached && 
                KmdfUsbCheckTxBufferCnt(&device->usbCb.USB_TX_PIPE[epId]) > 0) {
                //KmdfUsbSubmitTxData(device->os_hdl, &device->usbCb.USB_TX_PIPE[epId], epId);
            }
        } else {
            for (i = 0; i < ATH_USB_RX_CELL_NUMBER; i++) {
                NdisAcquireSpinLock(&pUsbCb->UsbRxListLock);
                freeObj = WdfCollectionGetFirstItem(pUsbCb->UsbRxFreeList);
                if (freeObj == NULL) {
                    NdisReleaseSpinLock(&pUsbCb->UsbRxListLock);
                    continue;
                }
                
                WdfCollectionRemove(pUsbCb->UsbRxFreeList, freeObj);
                WdfCollectionAdd(pUsbCb->UsbRxUsedList, freeObj);
                
                NdisReleaseSpinLock(&pUsbCb->UsbRxListLock);
                
                SendRxObject(device, freeObj);
            }
        }
    }    
            
    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: epId = %d, ntStatus = 0x%x\n",  __FUNCTION__, __LINE__, epId, ntStatus));        
    
    return ntStatus;
}

void
KmdfUsbPipeResetRoutine(
    IN PVOID  WorkItemContext,
    IN NDIS_HANDLE  NdisIoWorkItemHandle
)
{
#define USB_EPRESET_DELAY_TIME  1000
#define USB_EPRESET_TRY_CNT     5

    PUsbPipeResetInfo   pUsbPipeResetInfo = (PUsbPipeResetInfo)WorkItemContext;
    HIF_DEVICE_USB *device = (HIF_DEVICE_USB *)pUsbPipeResetInfo->device;
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    long                epId = pUsbPipeResetInfo->epId;
    NTSTATUS            ntStatus;
    int                 i, j;
    WDFOBJECT           freeObj = NULL;
    WDFUSBPIPE          hPipe = NULL;

    do {        
        if (epId >= HIF_USB_PIPE_MAX) {
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: Invalid epId(%d)\n",  __FUNCTION__, __LINE__, epId));        
            break;
        }
        
        if (MP_GET_BIT(pUsbCb->UsbPipeResetFlag, epId) == 0) {
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: Reset bit doesn't be setted(epId = %d, UsbPipeResetFlag = 0x%x)\n",  __FUNCTION__, __LINE__, epId, pUsbCb->UsbPipeResetFlag));        
            break;
        }
        
        if (device->bUSBDeveiceAttached == FALSE) {
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: USB device detached !, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
            MP_INTERLOCKED_CLR_BIT(pUsbCb->UsbPipeResetFlag, epId);
            break;
        }
      
        ntStatus = KmdfUsbPipeAbortSync(device, epId);
        if (ntStatus == STATUS_NO_SUCH_DEVICE) {
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: USB device detached !, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
            MP_INTERLOCKED_CLR_BIT(pUsbCb->UsbPipeResetFlag, epId);
            break;
        }
        
        ntStatus = KmdfUsbPipeResetSync(device, epId);       
        if (ntStatus == STATUS_NO_SUCH_DEVICE) {
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: USB device detached !, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
            MP_INTERLOCKED_CLR_BIT(pUsbCb->UsbPipeResetFlag, epId);
            break;
        }
        
        MP_INTERLOCKED_CLR_BIT(pUsbCb->UsbPipeResetFlag, epId);
        
        KmdfUsbPipeSubmitUrb(device, epId);
                              
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: USB endpoint reset complete, epId = %d\n",  __FUNCTION__, __LINE__, epId));        
    } while (0);
    
    NdisFreeIoWorkItem(NdisIoWorkItemHandle);

#undef USB_EPRESET_TRY_CNT
#undef USB_EPRESET_DELAY_TIME
}

USHORT KmdfUsbCheckTxBufferCnt(HIF_USB_TX_PIPE *txPipe)
{
    USHORT txBufCnt;

    NdisAcquireSpinLock(&txPipe->UsbTxBufQLock);
    txBufCnt = txPipe->TxBufCnt;
    NdisReleaseSpinLock(&txPipe->UsbTxBufQLock); 

    return txBufCnt;
}
void HIF_InitTxPipe(HIF_DEVICE_USB *device, UCHAR endpt)
{
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;    
    WDF_USB_PIPE_INFORMATION pipInfo;
    	    
    printk("InitTxPipe\n");

    pUsbCb->USB_TX_PIPE[endpt].UsbTxPipe = WdfUsbInterfaceGetConfiguredPipe(pUsbCb->UsbInterface,
                                                             endpt + 4, // PipeIndex,
                                                             NULL); // pipeInfo  

    WdfUsbTargetPipeGetInformation(pUsbCb->USB_TX_PIPE[endpt].UsbTxPipe,
        &pUsbCb->USB_TX_PIPE[endpt].UsbTxPipeInfo);
}

A_STATUS KmdfUsbPutTxBuffer(HIF_DEVICE_USB *device, UCHAR endpt, unsigned int transfer_id, adf_nbuf_t buf)
{
    UsbTxQ_t *TxQ;
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    HIF_USB_TX_PIPE *txPipe = &(pUsbCb->USB_TX_PIPE[endpt]);
    systime_t sysTime;

    NdisAcquireSpinLock(&txPipe->UsbTxBufQLock);


    if (txPipe->TxBufCnt < ATH_USB_MAX_TX_BUF_NUM) {
        TxQ = (UsbTxQ_t *)&(txPipe->UsbTxBufQ[txPipe->TxBufTail]);
        TxQ->buf = buf;
        NdisGetSystemUpTimeEx((PLARGE_INTEGER)&sysTime);
        TxQ->timeStamp = sysTime;

        txPipe->TxBufTail = ((txPipe->TxBufTail+1) & (ATH_USB_MAX_TX_BUF_NUM - 1));
        txPipe->TxBufCnt++;

        if(txPipe->UsbTxPipeInfo.EndpointAddress != 1) {
            NdisInterlockedIncrement((PLONG)&(txPipe->usbTxPktCnt));
        }
    }
    else
    {
        NdisReleaseSpinLock(&txPipe->UsbTxBufQLock); 
        if (buf != NULL) {
              AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s because HIFQ is full and then drop frame \n",__FUNCTION__));
              device->htcCallbacks.txCompletionHandler(device->htcCallbacks.Context,
                                                                         buf, transfer_id);
        }
        return A_NO_RESOURCE;
    }

    NdisReleaseSpinLock(&txPipe->UsbTxBufQLock); 

    return A_OK;
}

NTSTATUS KmdfUsbGetTxBuffer(HIF_USB_TX_PIPE *txPipe, UsbTxQ_t *TxQ)
{
    NTSTATUS status = NDIS_STATUS_SUCCESS;

    NdisAcquireSpinLock(&txPipe->UsbTxBufQLock);
    if (txPipe->TxBufCnt > 0)
    {
        NdisMoveMemory(TxQ, &(txPipe->UsbTxBufQ[txPipe->TxBufHead]), sizeof(UsbTxQ_t));
        txPipe->TxBufHead = ((txPipe->TxBufHead+1) & (ATH_USB_MAX_TX_BUF_NUM - 1));
        txPipe->TxBufCnt--;
    }
    else
        status = NDIS_STATUS_FAILURE;

    NdisReleaseSpinLock(&txPipe->UsbTxBufQLock); 
    
    return status;
}

static WDFOBJECT allocateTxObject(HIF_USB_TX_PIPE *txPipe, USHORT *pBufLen, unsigned int transfer_id)
{
    WDFOBJECT txObj = NULL;    
    UsbOutRequestItem *pItem;
    UCHAR *pBuf;
    NTSTATUS status;
    UsbTxQ_t TxQ;
    HIF_DEVICE_USB              *device = NULL;
    NdisAcquireSpinLock(&txPipe->UsbTxListLock);
    txObj = WdfCollectionGetFirstItem(txPipe->UsbTxFreeList);

    if (txObj != NULL) {
        WdfCollectionRemove(txPipe->UsbTxFreeList, txObj);
        status = WdfCollectionAdd(txPipe->UsbTxUsedList, txObj);
        ASSERT(status == STATUS_SUCCESS);
    }
    
    NdisReleaseSpinLock(&txPipe->UsbTxListLock);

    if (txObj == NULL) {
        return NULL;
    }

    status = KmdfUsbGetTxBuffer(txPipe, &TxQ);

    if (status != NDIS_STATUS_SUCCESS)
    {
        NdisAcquireSpinLock(&txPipe->UsbTxListLock);
        WdfCollectionRemove(txPipe->UsbTxUsedList, txObj);
        WdfCollectionAdd(txPipe->UsbTxFreeList, txObj);
        NdisReleaseSpinLock(&txPipe->UsbTxListLock);

        return NULL;
    }

    pItem = (UsbOutRequestItem *)GetUsbOutRequestItem(txObj);
    pItem->IsUsed = 1;
    pItem->transfer_id = transfer_id;
#if AR6004_HW
{
    UCHAR *pBuf = NULL;
    UCHAR *buf =  adf_nbuf_data(TxQ.buf);
    EPPING_HEADER *pktHeader = NULL;
    pBuf = (UCHAR *)WdfMemoryGetBuffer(pItem->UsbRequestBuffer, NULL);
    if ((adf_nbuf_len(TxQ.buf) == 20) && (*(buf+8) == 5)) {
     *(buf+2) = 0xA;
     A_MEMCPY(pBuf,
              buf,
              6);
     A_MEMCPY(pBuf+6,
              (buf+8),
              2);
     A_MEMCPY(pBuf+8,
              (buf+12),
              (adf_nbuf_len(TxQ.buf) -4));
     *pBufLen = adf_nbuf_len(TxQ.buf) - 4;
    } else {
       pktHeader = (EPPING_HEADER *)(buf+8);
       if (IS_EPPING_PACKET(pktHeader)) {
          status = WdfMemoryAssignBuffer(pItem->UsbRequestBuffer, 
                                       adf_nbuf_data(TxQ.buf),
                                       adf_nbuf_len(TxQ.buf));

          *pBufLen = adf_nbuf_len(TxQ.buf);
       } else {
           A_MEMCPY(pBuf,
                  buf,
                  6);
           A_MEMCPY(pBuf+6,
                  (buf+8),
                  (adf_nbuf_len(TxQ.buf) -8));
           *pBufLen = adf_nbuf_len(TxQ.buf) - 2;
       }
    }

}
#else
    status = WdfMemoryAssignBuffer(pItem->UsbRequestBuffer, 
                                   adf_nbuf_data(TxQ.buf),
                                   adf_nbuf_len(TxQ.buf));

    *pBufLen = adf_nbuf_len(TxQ.buf);
#endif
    pItem->tx_adf_buf = TxQ.buf;
/*
    device = pItem->sc->UsbBusInterface;
    device->htcCallbacks.txCompletionHandler(device->htcCallbacks.Context,
                                                      TxQ.buf, transfer_id);
*/
    return txObj;
}


static WDFOBJECT allocateZeroTxObject(HIF_USB_TX_PIPE *txPipe)
{
    WDFOBJECT           zeroTxObj;
    UsbZeroOutRequestItem   *pItem;
    NTSTATUS            status;
    PURB                pUrb = NULL;
    UCHAR               buf[1];
    
    NdisAcquireSpinLock(&txPipe->UsbTxListLock);
    zeroTxObj = WdfCollectionGetFirstItem(txPipe->UsbTxZeroFreeList);
    
    if (zeroTxObj != NULL)
    {
        WdfCollectionRemove(txPipe->UsbTxZeroFreeList, zeroTxObj);
        status = WdfCollectionAdd(txPipe->UsbTxZeroUsedList, zeroTxObj);
        ASSERT(status == STATUS_SUCCESS);
    }
    
    NdisReleaseSpinLock(&txPipe->UsbTxListLock);
    
    if (zeroTxObj == NULL)
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("allocateZeroTxObject: No More Tx Request Object\n"));
        return NULL;
    }
    
    pItem = (UsbZeroOutRequestItem *)GetUsbZeroOutRequestItem(zeroTxObj);
    pItem->IsUsed = 1;
    
    pUrb = (PURB)WdfMemoryGetBuffer(pItem->UsbUrbBuffer, NULL);
    UsbBuildInterruptOrBulkTransferRequest(
        pUrb,                                         // URB
        (USHORT) sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),  // size of URB
        WdfUsbTargetPipeWdmGetPipeHandle(txPipe->UsbTxPipe),                                        // Endpoint pipe handle
        buf,                                                      // buffer to be sent                            
        NULL,                                                     // transfer buffer (MDL)
        0,                                                        // size of buffer
        USBD_TRANSFER_DIRECTION_OUT,                              // transfer direction
        NULL                                                      // link
    );

    return zeroTxObj;
}

A_UINT8 usb_hif_get_usb_tx_pipe_id(A_UINT8 index)
{
    A_UINT8 pipe_id = 0;

    switch (index) {
        case 0:
            pipe_id = HIF_TX_CTRL_PIPE;
            break;
        case 1:
            pipe_id = HIF_TX_DATA_LP_PIPE;
            break;
        case 2:
            pipe_id = HIF_TX_DATA_MP_PIPE;
            break;
        case 3:
            pipe_id = HIF_TX_DATA_HP_PIPE;
            break;
        default :
            /* note: there may be endpoints not currently used */
            break;
    }

    return pipe_id;
}
#if defined(EPPING_TEST)
extern unsigned int txCompleteCounter;
#endif

void UsbTxRequestCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
{
    NTSTATUS    status;
    WDF_REQUEST_REUSE_PARAMS params;
    WDFOBJECT txObj = (WDFOBJECT)Context;
    UsbOutRequestItem *pItem = (UsbOutRequestItem *)GetUsbOutRequestItem(txObj);
    HIF_DEVICE_USB *device = (HIF_DEVICE_USB *)pItem->sc->UsbBusInterface;
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    HIF_USB_TX_PIPE *txPipe = &(pUsbCb->USB_TX_PIPE[pItem->PipeIndex]);
    NTSTATUS    txStatus;
#if defined(EPPING_TEST)
    UCHAR *pBuf = NULL;
    EPPING_HEADER *pktHeader = NULL;
#endif
    txStatus = CompletionParams->IoStatus.Status;
    
    if ( MP_GET_BIT(pUsbCb->UsbPipeResetFlag, usb_hif_get_usb_tx_pipe_id (pItem->PipeIndex))) {
    } else if (!NT_SUCCESS(txStatus)) {
        if ( (txStatus != STATUS_CANCELLED) &&
             (txStatus != STATUS_NO_SUCH_DEVICE) &&
             (device->bUSBDeveiceAttached == TRUE) ) {
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: Issue reset ! , IoStatus = 0x%x, UsbdStatus = 0x%x\n", __FUNCTION__, __LINE__, txStatus, CompletionParams->Parameters.Usb.Completion->UsbdStatus));        
            KmdfUsbSchedulePipeReset(device, pUsbCb->USBRestPipe[usb_hif_get_usb_tx_pipe_id (pItem->PipeIndex)]);
        }
    }

    WDF_REQUEST_REUSE_PARAMS_INIT(&params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);

    status = WdfRequestReuse(pItem->UsbOutRequest, &params);
    if (!NT_SUCCESS(status))
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: WdfRequestReuse fail\n", __FUNCTION__));
        ASSERT(0);
    }  
    

    if (1)
    {
        NdisAcquireSpinLock(&txPipe->UsbTxListLock);
        
        device->htcCallbacks.txCompletionHandler(device->htcCallbacks.Context,
                                                pItem->tx_adf_buf, pItem->transfer_id);

#if defined(EPPING_TEST)
        pBuf = (UCHAR *)WdfMemoryGetBuffer(pItem->UsbRequestBuffer, NULL);
        pktHeader = (EPPING_HEADER *)(pBuf + HTC_HDR_LENGTH);
        if (IS_EPPING_PACKET(pktHeader)){
            txCompleteCounter++;
        }
#endif
        pItem->tx_adf_buf = NULL;
        pItem->IsUsed = 0;
        pItem->IsReboot = 0;

        WdfCollectionRemove(txPipe->UsbTxUsedList, txObj);
        status = WdfCollectionAdd(txPipe->UsbTxFreeList, txObj);
        ASSERT(status == STATUS_SUCCESS);
        NdisReleaseSpinLock(&txPipe->UsbTxListLock);
    }
    else
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: FATAL ERROR!! pItem->IsUsed is zero\n", __FUNCTION__));
    }

    if (NT_SUCCESS(txStatus) &&  device->bUSBDeveiceAttached && KmdfUsbCheckTxBufferCnt(txPipe) > 0) {
        KmdfUsbSubmitTxData(device, txPipe, pItem->PipeIndex, pItem->transfer_id);
    }    
}

NTSTATUS doUsbSend(HIF_USB_TX_PIPE *txPipe, WDFOBJECT txObj, UsbOutRequestItem *pItem, int bufLen)
{
    WDFMEMORY_OFFSET wdfMemOffset;
    NTSTATUS status;
    BOOLEAN reqSentRes;
    
    wdfMemOffset.BufferOffset = 0;
    wdfMemOffset.BufferLength = bufLen;
    status = WdfUsbTargetPipeFormatRequestForWrite(txPipe->UsbTxPipe, 
                                                   pItem->UsbOutRequest,
                                                   pItem->UsbRequestBuffer,
                                                   &wdfMemOffset);
    if (!NT_SUCCESS(status))
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: WdfUsbTargetPipeFormatRequestForWrite\n", __FUNCTION__));
        goto send_failed;
    }
                                              
    WdfRequestSetCompletionRoutine(pItem->UsbOutRequest, 
                                   UsbTxRequestCompletionRoutine, txObj);

    reqSentRes = WdfRequestSend(pItem->UsbOutRequest,
                                WdfUsbTargetPipeGetIoTarget(txPipe->UsbTxPipe),
                                NULL);
    if (reqSentRes == FALSE) 
    {
        WDF_REQUEST_REUSE_PARAMS params;        
            
        WDF_REQUEST_REUSE_PARAMS_INIT(&params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
        WdfRequestReuse(pItem->UsbOutRequest, &params);
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: WdfRequestSend\n", __FUNCTION__));
        status = STATUS_UNSUCCESSFUL;
        goto send_failed;
    }
    
    return STATUS_SUCCESS;
                       
send_failed:
    return status;
}
void UsbTxZeroRequestCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
{
    NTSTATUS    status;
    WDF_REQUEST_REUSE_PARAMS params;
    WDFOBJECT txObj = (WDFOBJECT)Context;
    UsbZeroOutRequestItem *pItem = (UsbZeroOutRequestItem *)GetUsbZeroOutRequestItem(txObj);
    HIF_DEVICE_USB *device = (HIF_DEVICE_USB *)pItem->sc->UsbBusInterface;
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    HIF_USB_TX_PIPE *txPipe = &(pUsbCb->USB_TX_PIPE[pItem->PipeIndex]);
    NTSTATUS    txStatus;

    txStatus = CompletionParams->IoStatus.Status;
    
    if ( MP_GET_BIT(pUsbCb->UsbPipeResetFlag, usb_hif_get_usb_tx_pipe_id (pItem->PipeIndex)) ){
    } else if (!NT_SUCCESS(txStatus)) {
        if ( (txStatus != STATUS_CANCELLED) &&
             (txStatus != STATUS_NO_SUCH_DEVICE) &&
             (device->bUSBDeveiceAttached == TRUE) ) {
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s_%d: Issue reset ! , IoStatus = 0x%x, UsbdStatus = 0x%x\n", __FUNCTION__, __LINE__, txStatus, CompletionParams->Parameters.Usb.Completion->UsbdStatus));        
            KmdfUsbSchedulePipeReset(device, pUsbCb->USBRestPipe[usb_hif_get_usb_tx_pipe_id (pItem->PipeIndex)]);
        }
    }

    WDF_REQUEST_REUSE_PARAMS_INIT(&params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);

    status = WdfRequestReuse(pItem->UsbOutRequest, &params);
    if (!NT_SUCCESS(status))
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: WdfRequestReuse fail\n", __FUNCTION__));
        ASSERT(0);
    }  

    if (1)
    {
        NdisAcquireSpinLock(&txPipe->UsbTxListLock);        
        pItem->IsUsed = 0;
        pItem->IsReboot = 0;

        WdfCollectionRemove(txPipe->UsbTxZeroUsedList, txObj);
        status = WdfCollectionAdd(txPipe->UsbTxZeroFreeList, txObj);
        ASSERT(status == STATUS_SUCCESS);
        NdisReleaseSpinLock(&txPipe->UsbTxListLock);
    }
    else
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: FATAL ERROR!! pItem->IsUsed is zero\n", __FUNCTION__));
    }


  
}

static NTSTATUS doUsbSendUrb(HIF_USB_TX_PIPE *txPipe, WDFOBJECT txObj, UsbZeroOutRequestItem *pItem, int bufLen)
{
    WDFMEMORY_OFFSET wdfMemOffset;
    NTSTATUS status;
    BOOLEAN reqSentRes;
    
    wdfMemOffset.BufferOffset = 0;
    wdfMemOffset.BufferLength = bufLen;
    status = WdfUsbTargetPipeFormatRequestForUrb(txPipe->UsbTxPipe, 
                                                   pItem->UsbOutRequest,
                                                   pItem->UsbUrbBuffer,
                                                   &wdfMemOffset);
    if (!NT_SUCCESS(status))
    {
        goto send_failed;
    }
                                              
    WdfRequestSetCompletionRoutine(pItem->UsbOutRequest, 
                                   UsbTxZeroRequestCompletionRoutine, txObj);
                                                      
    reqSentRes = WdfRequestSend(pItem->UsbOutRequest,
                                WdfUsbTargetPipeGetIoTarget(txPipe->UsbTxPipe),
                                NULL);

    if (reqSentRes == FALSE)
    {
        WDF_REQUEST_REUSE_PARAMS params;        
            
        WDF_REQUEST_REUSE_PARAMS_INIT(&params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
        WdfRequestReuse(pItem->UsbOutRequest, &params);
        status = STATUS_UNSUCCESSFUL;
        goto send_failed;
    }
    
    return STATUS_SUCCESS;
                       
send_failed:

    return status;
}

A_STATUS KmdfUsbSubmitTxData(HIF_DEVICE_USB *device, HIF_USB_TX_PIPE *txPipe, LONG epId,unsigned int transfer_id)
{
    NTSTATUS status;
    WDFMEMORY_OFFSET wdfMemOffset;
    HIF_KMDF_USB_CB *pUsbCb = &device->usbCb;
    WDFOBJECT txObj;
    UsbOutRequestItem *pItem = NULL;
    UsbZeroOutRequestItem *pZeroItem = NULL;
    USHORT bufLen;
    WDFOBJECT zeroTxObj = NULL;

    if (!device->bUSBDeveiceAttached) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("%s: send when the device detached\n",
               __FUNCTION__));
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    else if (MP_GET_BIT(pUsbCb->UsbPipeResetFlag, epId)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("%s_%d: In USB pipe resetting, UsbPipeResetFlag = 0x%x\n", __FUNCTION__, __LINE__, pUsbCb->UsbPipeResetFlag));
        return 0;
    } 
    else if (MP_GET_BIT(pUsbCb->UsbPipeStopFlag, epId)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("%s_%d: In wlan resetting, UsbPipeStopFlag = 0x%x\n", __FUNCTION__, __LINE__, pUsbCb->UsbPipeStopFlag));
        return 0;        
    }  

    NdisAcquireSpinLock(&txPipe->UsbTxSendLock);
    txObj = allocateTxObject(txPipe, &bufLen, transfer_id);

    /* TODO: check whether we need to free buffer here */
    if (txObj == NULL) {
        NdisReleaseSpinLock(&txPipe->UsbTxSendLock);
        /* Always return success to avoid buffer double free */
        return 0;
    }

    if (((txPipe->UsbTxPipeInfo.MaximumPacketSize-1) & bufLen) == 0)
    {
        zeroTxObj = allocateZeroTxObject(txPipe);
        if (zeroTxObj == NULL) {
            NdisReleaseSpinLock(&txPipe->UsbTxSendLock);
            goto send_failed;
        }
    }

    //NdisAcquireSpinLock(&txPipe->UsbTxSendLock);
    pItem = (UsbOutRequestItem *)GetUsbOutRequestItem(txObj);    
    status = doUsbSend(txPipe, txObj, pItem, bufLen);

    if (!NT_SUCCESS(status)) {
        NdisReleaseSpinLock(&txPipe->UsbTxSendLock);
        goto send_failed;
    }

    if (zeroTxObj != NULL)
    {
        pZeroItem = (UsbZeroOutRequestItem *)GetUsbZeroOutRequestItem(zeroTxObj);    
        status = doUsbSendUrb(txPipe, zeroTxObj, pZeroItem, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));    
        if (!NT_SUCCESS(status))
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("doUsbSendUrb fail, status: %08x\n", status));
            ASSERT(0);
        }
    }

    NdisReleaseSpinLock(&txPipe->UsbTxSendLock);
    
    return NDIS_STATUS_SUCCESS;
    
send_failed:
    NdisAcquireSpinLock(&txPipe->UsbTxListLock);
    WdfCollectionRemove(txPipe->UsbTxUsedList, txObj); 
    status = WdfCollectionAdd(txPipe->UsbTxFreeList, txObj);
    ASSERT(status == STATUS_SUCCESS);       
    
    if (zeroTxObj != NULL)
    {
        WdfCollectionRemove(txPipe->UsbTxZeroUsedList, zeroTxObj); 
        status = WdfCollectionAdd(txPipe->UsbTxZeroFreeList, zeroTxObj);
        ASSERT(status == STATUS_SUCCESS);       
    }
    
    NdisReleaseSpinLock(&txPipe->UsbTxListLock);        
    return 0;
}

#endif
