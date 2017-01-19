#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <asm/ifx/ifx_regs.h>

extern int strncasecmp(const char *s1, const char *s2, size_t n);

#define NUM_ENTITY(x)       (sizeof(x) / sizeof(*x))
#define dprint              printk
#define print_u8(x)         printk("%u", (x))
#define REG32(x)            (*(volatile unsigned int *)(x))
#define BSP_GPIO_P0_OUT     IFX_GPIO_P0_OUT
#define BSP_GPIO_P0_IN      IFX_GPIO_P0_IN
#define BSP_GPIO_P0_ALTSEL0 IFX_GPIO_P0_ALTSEL0
#define BSP_GPIO_P0_ALTSEL1 IFX_GPIO_P0_ALTSEL1
#define BSP_GPIO_P0_OD      IFX_GPIO_P0_OD
#define BSP_GPIO_P0_PUDEN   IFX_GPIO_P0_PUDEN
#define BSP_GPIO_P0_DIR     IFX_GPIO_P0_DIR
#define BSP_GPIO_P1_OUT     IFX_GPIO_P1_OUT
#define BSP_GPIO_P1_IN      IFX_GPIO_P1_IN
#define BSP_GPIO_P1_ALTSEL0 IFX_GPIO_P1_ALTSEL0
#define BSP_GPIO_P1_ALTSEL1 IFX_GPIO_P1_ALTSEL1
#define BSP_GPIO_P1_OD      IFX_GPIO_P1_OD
#define BSP_GPIO_P1_DIR     IFX_GPIO_P1_DIR
#define OUT                 1

static void gpio_port_1_cfg(u32 gpio_num, int dir, u32 func_num)
{
    unsigned long flags;
    u32 wr_one,wr_zero;
    wr_one = 0x00000001;
    wr_one = wr_one << gpio_num;
    wr_zero = 0xFFFFFFFF - wr_one;
    ///ALT
    if (func_num == 0 || func_num == 2)
    { //writing zero
        local_irq_save(flags);
        REG32(BSP_GPIO_P1_ALTSEL0) = (REG32(BSP_GPIO_P1_ALTSEL0) & wr_zero);
        local_irq_restore(flags);
    }
    else
    { //writing one
        local_irq_save(flags);
        REG32(BSP_GPIO_P1_ALTSEL0) = (REG32(BSP_GPIO_P1_ALTSEL0) | wr_one);
        local_irq_restore(flags);
    };
    if (func_num == 0 || func_num == 1)
    { //writing zero
        local_irq_save(flags);
        REG32(BSP_GPIO_P1_ALTSEL1) = (REG32(BSP_GPIO_P1_ALTSEL1) & wr_zero);
        local_irq_restore(flags);
    }
    else
    { //writing one
        local_irq_save(flags);
        REG32(BSP_GPIO_P1_ALTSEL1) = (REG32(BSP_GPIO_P1_ALTSEL1) | wr_one);
        local_irq_restore(flags);
    };
    //Direction
    if (dir == OUT)
    {
        local_irq_save(flags);
        REG32(BSP_GPIO_P1_DIR) = REG32(BSP_GPIO_P1_DIR) | wr_one;
        REG32(BSP_GPIO_P1_OD) = REG32(BSP_GPIO_P1_OD) | wr_one;
        local_irq_restore(flags);
    }
    else
    {
        local_irq_save(flags);
        REG32(BSP_GPIO_P1_DIR) = REG32(BSP_GPIO_P1_DIR) & wr_zero;
        local_irq_restore(flags);
    }
}


//////////////////////////////////////////
//wangyunl,2011.04.13
//#include <IIC.h>
//#include <gpio.h>

u32 Cal_Val = 0x19a;
u32 Con_Val = 0x1fff;


static void SomeNOP(void)
{
    volatile int i;
    for (i=0;i<1;i++)
    {}
}


static void SDA_OUT_INA219(unsigned char data)
{
    unsigned long flags;
    unsigned char Temp;
    int i;
    Temp = (REG32(BSP_GPIO_P1_OUT) & 0x200)>>9;

    if(Temp == data)
    {
    }
    else
    {
        if(Temp==0)
        {
            local_irq_save(flags);
            REG32(BSP_GPIO_P1_OUT) |= 0x200;
            local_irq_restore(flags);
        }
        else
        {
            local_irq_save(flags);
            REG32(BSP_GPIO_P1_OUT) &= ~(0x200);
            local_irq_restore(flags);
        }

    }

    for (i=0;i<100;i++)
    {
        SomeNOP();
    }
}



static void SDA_OUT(unsigned char data)
{
    unsigned long flags;
    unsigned char Temp;
    Temp = (REG32(BSP_GPIO_P0_OUT) & 0x200)>>9;

    if(Temp == data)
    {
        return;
    }
    else
    {
        if(Temp==0)
        {
            local_irq_save(flags);
            REG32(BSP_GPIO_P0_OUT) |= 0x200;
            local_irq_restore(flags);
        }
        else
        {
            local_irq_save(flags);
            REG32(BSP_GPIO_P0_OUT) &= ~(0x200);
            local_irq_restore(flags);
        }
        return;
    }
}

static unsigned char SDA_IN_INA219(void)
{
    unsigned char Temp;
    Temp = (REG32(BSP_GPIO_P1_IN) & 0x200)>>9;

    return Temp;
}


static unsigned char SDA_IN(void)
{
    unsigned char Temp;
    Temp = (REG32(BSP_GPIO_P0_IN) & 0x200)>>9;

    return Temp;
}

static void SCL_OUT_INA219(unsigned char data)
{
    unsigned long flags;
    int i;
    unsigned char Temp;
    Temp = (REG32(BSP_GPIO_P1_OUT) & 0x400)>>10;

    if(Temp == data)
    {
    }
    else
    {
        if(Temp==0)
        {
            local_irq_save(flags);
            REG32(BSP_GPIO_P1_OUT) |= 0x400;
            local_irq_restore(flags);
        }
        else
        {
            local_irq_save(flags);
            REG32(BSP_GPIO_P1_OUT) &= ~(0x400);
            local_irq_restore(flags);
        }

    }
    for(i=0;i<1000;i++)
    {
        SomeNOP();
    }

    return;
}

static void SCL_OUT(unsigned char data)
{
    unsigned long flags;
    unsigned char Temp;
    Temp = (REG32(BSP_GPIO_P0_OUT) & 0x800)>>11;

    if(Temp == data)
    {
        return;
    }
    else
    {
        if(Temp==0)
        {
            local_irq_save(flags);
            REG32(BSP_GPIO_P0_OUT) |= 0x800;
            local_irq_restore(flags);
        }
        else
        {
            local_irq_save(flags);
            REG32(BSP_GPIO_P0_OUT) &= ~(0x800);
            local_irq_restore(flags);
        }
        return;
    }
}

static void I2CStart_INA219(void)
{
    int i;
    for(i=0;i<100;i++)
    {
        SomeNOP();
    }

    SDA_OUT_INA219(0);

    for(i=0;i<100;i++)
    {
        SomeNOP();
    }

    SCL_OUT_INA219(0);
    for(i=0;i<100;i++)
    {
        SomeNOP();
    }
}

static void I2CStart(void)
{
    int i;

    SDA_OUT(0);
    for(i=0;i<100;i++)
    {
        SomeNOP();
    }

    SCL_OUT(0);
    for(i=0;i<100;i++)
    {
        SomeNOP();
    }
}

static void I2CStop_INA219(void)
{
    int i;
    SCL_OUT_INA219(0);
    SomeNOP();
    SDA_OUT_INA219(0);
    SomeNOP();

    SCL_OUT_INA219(1);
    for(i=0;i<100;i++)
    {
        SomeNOP();
    }

    SDA_OUT_INA219(1);
    for(i=0;i<100;i++)
    {
        SomeNOP();
    }
}

static void I2CStop(void)
{
    SCL_OUT(0);
    SomeNOP();
    SDA_OUT(0);
    SomeNOP();

    SCL_OUT(1);
    SomeNOP();
    SDA_OUT(1);
    SomeNOP();
}


static bool WaitAck_INA219(void)
{
    unsigned int errtime=8000;
//    int i;

    SCL_OUT_INA219(1);
    SomeNOP();


    while(SDA_IN_INA219())
    {
        errtime--;
        if (!errtime) {
            I2CStop_INA219();

            return 0;
        }
    }

    SomeNOP();

    SCL_OUT_INA219(0);
    SomeNOP();

    return 1;
}


static bool WaitAck(void)
{
    unsigned int errtime=8000;

    SCL_OUT(1);
    SomeNOP();


    while(SDA_IN())
    {
        errtime--;
        if (!errtime) {
            I2CStop();
            return 0;
        }
    }

    SomeNOP();

    SCL_OUT(0);
    SomeNOP();

    return 1;
}

static void SendAck_INA219(void)
{
    SCL_OUT_INA219(0);
    SomeNOP();

    SDA_OUT_INA219(0);
    SomeNOP();

    SCL_OUT_INA219(1);
    SomeNOP();

    SCL_OUT_INA219(0);
    SomeNOP();

    SDA_OUT_INA219(1);
    SomeNOP();

}

static void SendNotAck_INA219(void)
{
    SCL_OUT_INA219(0);
    SomeNOP();

    SDA_OUT_INA219(1);
    SomeNOP();

    SCL_OUT_INA219(1);
    SomeNOP();

    SCL_OUT_INA219(0);
    SomeNOP();

}


static void SendNotAck(void)
{
    SCL_OUT(0);
    SomeNOP();

    SDA_OUT(1);
    SomeNOP();

    SCL_OUT(1);
    SomeNOP();

    SCL_OUT(0);
    SomeNOP();

}

static void I2CSendByte(unsigned char ch)
{
    unsigned char i=8,Temp;

    while (i--)
    {
        SCL_OUT(0);
        SomeNOP();

        Temp=(ch & 0x80)>>7;
        SDA_OUT(Temp);
        ch=ch<<1;
        SomeNOP();

        SCL_OUT(1);
        SomeNOP();

    }
    SCL_OUT(0);
    SomeNOP();

    SDA_OUT(1);
    SomeNOP();

}

static void I2CSendByte_INA219(unsigned char ch)
{
    unsigned char i=8,Temp;

    while (i--)
    {
        SCL_OUT_INA219(0);
        SomeNOP();

        Temp=(ch & 0x80)>>7;
        SDA_OUT_INA219(Temp);
        ch=ch<<1;
        SomeNOP();

        SCL_OUT_INA219(1);
        SomeNOP();

    }
    SCL_OUT_INA219(0);
    SomeNOP();


    SDA_OUT_INA219(1);
    SomeNOP();

}

//////////////////////
static unsigned char  I2CReceiveByte_INA219(void)
{

    unsigned char i=8,j;
    unsigned char ddata=0;


    SCL_OUT_INA219(0);
    SomeNOP();


    while (i--)
    {

        ddata = ddata << 0x1;
        SCL_OUT_INA219(0);
        SomeNOP();

        SCL_OUT_INA219(1);

        for(j=0;j<100;j++)
        {
            SomeNOP();
        }

        ddata|=(0x1 & SDA_IN_INA219());
        SomeNOP();

        for(j=0;j<100;j++)
        {
            SomeNOP();
        }

        SCL_OUT_INA219(0);
        SomeNOP();
    }

    return ddata;

}



static unsigned char  I2CReceiveByte(void)
{

    unsigned char i=8;
    unsigned char ddata=0;

    SCL_OUT(0);
    SomeNOP();

    SDA_OUT(0);
    SomeNOP();


    SDA_OUT(1);
    SomeNOP();

    while (i--)
    {

        ddata = ddata << 0x1;
        SCL_OUT(0);
        SomeNOP();

        SCL_OUT(1);
        SomeNOP();

        ddata|=(0x1 & SDA_IN());
        SomeNOP();

        SCL_OUT(0);
        SomeNOP();


    }

    SDA_OUT(1);
    SomeNOP();

    return ddata;

}

static void GPIO_Setting_IIC_General(void)
{
    unsigned long flags;

    local_irq_save(flags);
    REG32(BSP_GPIO_P0_ALTSEL0) &= ~(0xA00);
    REG32(BSP_GPIO_P0_ALTSEL1) &= ~(0xA00);
    REG32(BSP_GPIO_P0_OD) &= ~(0xA00);
    REG32(BSP_GPIO_P0_PUDEN) &= ~(0xA00);
    local_irq_restore(flags);
}

static void GPIO_Setting_IIC_Master_Send_INA219_SDA_OUT(void)
{
    //master send mode:
    //SCL: OUT,GPIO26,P1.10
    //SDA: OUT,GPIO25,P1.9

    //SDA fall when set to output
    gpio_port_1_cfg(9,OUT,0); // GPIO 25 - GPIO,SDA - OUT
    SomeNOP();
}

static void GPIO_Setting_IIC_Master_Send_INA219(void)
{
    //master send mode:
    //SCL: OUT,GPIO26,P1.10
    //SDA: OUT,GPIO25,P1.9

    //SDA fall when set to output
    gpio_port_1_cfg(9,OUT,0); // GPIO 25 - GPIO,SDA - OUT
    SomeNOP();

    //SCL fall when set to output
    gpio_port_1_cfg(10,OUT,0); // GPIO 26 - GPIO,SCL - OUT
}


static void GPIO_Setting_IIC_Master_Send(void)
{
    unsigned long flags;

    //master send mode:
    //SCL: OUT,GPIO11,P0.11
    //SDA: OUT,GPIO9,P0.9

    //SDA fall when set to output
    local_irq_save(flags);
    REG32(BSP_GPIO_P0_DIR) |= (0x200);
    local_irq_restore(flags);
    SomeNOP();

    //SCL fall when set to output
    local_irq_save(flags);
    REG32(BSP_GPIO_P0_DIR) |= (0x800);
    local_irq_restore(flags);
}



static void GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN(void)
{
    unsigned long flags;

    //master receive mode:
    //SCL: OUT,GPIO10,P1.10
    //SDA: IN,GPIO9,P1.9
    local_irq_save(flags);
    REG32(BSP_GPIO_P1_DIR) &= ~(0x200);
    local_irq_restore(flags);
}


#if 0
static void GPIO_Setting_IIC_Master_Receive_INA219(void)
{
    unsigned long flags;

    //master receive mode:
    //SCL: OUT,GPIO10,P1.10
    //SDA: IN,GPIO9,P1.9
    local_irq_save(flags);
    REG32(BSP_GPIO_P1_DIR) &= ~(0x200);
    REG32(BSP_GPIO_P1_DIR) |= (0x400);
    local_irq_restore(flags);
}
#endif

static void GPIO_Setting_IIC_Master_Receive(void)
{
    unsigned long flags;

    //master receive mode:
    //SCL: OUT,GPIO11,P0.11
    //SDA: IN,GPIO9,P0.9
    local_irq_save(flags);
    REG32(BSP_GPIO_P0_DIR) &= ~(0x200);
    REG32(BSP_GPIO_P0_DIR) |= (0x800);
    local_irq_restore(flags);
}
////////////////////

#if 0
static void Master_Write_Data_INA219(unsigned char Char_Send)
{
    unsigned char TestAck;

    unsigned int errtime=1000;

    //Master Send
    GPIO_Setting_IIC_Master_Send_INA219();
    SomeNOP();

    I2CSendByte_INA219(Char_Send);

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219();
    SomeNOP();

    TestAck = WaitAck_INA219();

    /*
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }
    */


    while(1)
    {
        errtime--;
        if (!errtime) {
            break;
        }
    }
}
#endif

static void Master_Write_Data(unsigned char Char_Send)
{
    unsigned char TestAck;

    unsigned int errtime=1000;

    //Master Send
    GPIO_Setting_IIC_Master_Send();
    SomeNOP();

    I2CSendByte(Char_Send);

    //Master Receive
    GPIO_Setting_IIC_Master_Receive();
    SomeNOP();

    TestAck = WaitAck();

    /*
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }
    */


    while(1)
    {
        errtime--;
        if (!errtime)   {
            break;
        }
    }
}
//////////////////////////

static void Master_Register_Write_INA219(u32 Reg_Value)
{
    unsigned char TestAck;
    unsigned char Reg_Temp = 0;

    //////////////////////////////////////////
    Reg_Temp = (Reg_Value >> 8) & 0xff;
    //Master Send
    GPIO_Setting_IIC_Master_Send_INA219_SDA_OUT();
    SomeNOP();

    I2CSendByte_INA219(Reg_Temp);

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    TestAck = WaitAck_INA219();
    //////////////////////////////////////
    Reg_Temp = Reg_Value & 0xff;
    //Master Send
    GPIO_Setting_IIC_Master_Send_INA219_SDA_OUT();
    SomeNOP();

    I2CSendByte_INA219(Reg_Temp);

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    TestAck = WaitAck_INA219();
}


static void Master_Address_INA219(unsigned char Address)
{
    unsigned char TestAck;
    //Master Send
    GPIO_Setting_IIC_Master_Send_INA219_SDA_OUT();
    SomeNOP();

    I2CSendByte_INA219(Address);

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    TestAck = WaitAck_INA219();
    /*
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }
    */

}


static void Master_Address(unsigned char Address)
{
    unsigned char TestAck;
    //Master Send
    GPIO_Setting_IIC_Master_Send();
    SomeNOP();

    I2CSendByte(Address);

    //Master Receive
    GPIO_Setting_IIC_Master_Receive();
    SomeNOP();

    TestAck = WaitAck();
    /*
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }
    */

}
////////////////////////////

static unsigned char Master_Receive_Data_ACK_INA219(void)
{
    unsigned char Receive_Data = 0;

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    Receive_Data = I2CReceiveByte_INA219();

    GPIO_Setting_IIC_Master_Send_INA219_SDA_OUT();
    SomeNOP();

    SendAck_INA219();


    return Receive_Data;
}


static unsigned char Master_Receive_Data_ACK(void)
{
    unsigned char Receive_Data;
    unsigned char TestAck;

    //Master Receive
    GPIO_Setting_IIC_Master_Receive();
    SomeNOP();

    Receive_Data = I2CReceiveByte();

    TestAck = WaitAck();
    /*
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }
    */

    return Receive_Data;
}
/////////////////////////////////
static unsigned char Master_Receive_Data_NOTACK_INA219(void)
{
    unsigned char Receive_Data;


    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    Receive_Data = I2CReceiveByte_INA219();

    //Master Send
    GPIO_Setting_IIC_Master_Send_INA219_SDA_OUT();
    SomeNOP();

    SendNotAck_INA219();

    return Receive_Data;
}


static unsigned char Master_Receive_Data_NOTACK(void)
{
    unsigned char Receive_Data;


    //Master Receive
    GPIO_Setting_IIC_Master_Receive();
    SomeNOP();

    Receive_Data = I2CReceiveByte();

    //Master Send
    GPIO_Setting_IIC_Master_Send();
    SomeNOP();

    SendNotAck();

    return Receive_Data;
}
/////////////////////////////////


static void IIC_EEPROM_AT24C04_Acess(void)
{
    unsigned char i;
//    unsigned char count=10;
//    unsigned char *buff;
    unsigned char TestAck,Data_Rx[16];
    unsigned int errtime=3000;

    GPIO_Setting_IIC_General();
    //////////////////////////////////////device address///////////////////

    //Master Send
    GPIO_Setting_IIC_Master_Send();
    SomeNOP();

    I2CStart();
    I2CSendByte(0xA4);

    //Master Receive
    GPIO_Setting_IIC_Master_Receive();
    SomeNOP();

    TestAck = WaitAck();
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }

    //////////////////////////////////////word address///////////////////

    Master_Address(0x50);

    //////////////////////////////////////write data///////////////////

    for(i=0;i<16;i++)
    {
        Master_Write_Data(i);

    }

    ////////////////////////////////////////////////////////////////////
    //Master Send
    GPIO_Setting_IIC_Master_Send();
    SomeNOP();

    I2CStop();



    while(1)
    {
        errtime--;
        if (!errtime)   {
            break;
        }
    }


    //////////////////////////////////////device address,READ///////////////////

    //Master Send
    GPIO_Setting_IIC_Master_Send();
    SomeNOP();

    SCL_OUT(1);
    SomeNOP();


    I2CStart();
    I2CSendByte(0xA4);

    //Master Receive
    GPIO_Setting_IIC_Master_Receive();
    SomeNOP();

    TestAck = WaitAck();
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }

    //////////////////////////////////////word address///////////////////

    Master_Address(0x50);


    ////////////////////////////
    //Master Send
    GPIO_Setting_IIC_Master_Send();
    SomeNOP();

    SCL_OUT(1);
    SomeNOP();

    I2CStart();
    I2CSendByte(0xA5);



    //Master Receive
    GPIO_Setting_IIC_Master_Receive();
    SomeNOP();

    TestAck = WaitAck();
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }


    //////////////////////////////////////READ data///////////////////

    for(i=0;i<16;i++)
    {
        if(i!=15)
        {
            Data_Rx[i]=Master_Receive_Data_ACK();
        }
        else
        {
            Data_Rx[i]=Master_Receive_Data_NOTACK();
        }
    }

    ////////////////////////////////////////////////////////////////////
    //Master Send
    GPIO_Setting_IIC_Master_Send();
    SomeNOP();

    I2CStop();

    for(i=0;i<16;i++)
    {
        dprint("Write/Read data:");print_u8(Data_Rx[i]);dprint("\n");
    }

}

static void INA219_Register_Write_Dev_Addr_Value(unsigned char Dev_Addr,unsigned char Reg_Addr,u32 Reg_Value)
{
//    unsigned char i,count=10;
//    unsigned char *buff;
    unsigned char TestAck;
//    unsigned char Data_Rx[16];
//    unsigned int errtime=3000;
//    u32 Bus_Voltage = 0;

    unsigned char Dev_Addr_Byte_Write = 0x80;
//    unsigned char Dev_Addr_Byte_Read = 0x81;

    //GPIO_Setting_IIC_General();
    gpio_port_1_cfg(9,OUT,0); // GPIO 25 - GPIO,SDA - OUT,IN
    gpio_port_1_cfg(10,OUT,0); // GPIO 26 - GPIO,SCL - OUT
    //////////////////////////////////////device address///////////////////

    //Master Send
    GPIO_Setting_IIC_Master_Send_INA219();
    SomeNOP();

    Dev_Addr_Byte_Write &= ~(0x1e);
    Dev_Addr_Byte_Write |= ((Dev_Addr << 1) & 0x1e);

    I2CStart_INA219();
    I2CSendByte_INA219(Dev_Addr_Byte_Write); //10000100,

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    TestAck = WaitAck_INA219();

    //////////////////////////////////////word address///////////////////

    Master_Address_INA219(Reg_Addr);

    Master_Register_Write_INA219(Reg_Value);


    GPIO_Setting_IIC_Master_Send_INA219_SDA_OUT();
    SomeNOP();


    I2CStop_INA219();
}






static u32 INA219_Register_Read_Dev_Addr(unsigned char Dev_Addr,unsigned char Reg_Addr)
{
    unsigned char i;
//    unsigned char count=10;
//    unsigned char *buff;
    unsigned char TestAck,Data_Rx[16];
//    unsigned int errtime=3000;
    u32 Reg_Value = 0;
//    u32 Bus_Voltage = 0;

    unsigned char Dev_Addr_Byte_Write = 0x80;
    unsigned char Dev_Addr_Byte_Read = 0x81;

    //GPIO_Setting_IIC_General();
    gpio_port_1_cfg(9,OUT,0); // GPIO 25 - GPIO,SDA - OUT,IN
    gpio_port_1_cfg(10,OUT,0); // GPIO 26 - GPIO,SCL - OUT
    //////////////////////////////////////device address///////////////////

    //Master Send
    GPIO_Setting_IIC_Master_Send_INA219();
    SomeNOP();

    Dev_Addr_Byte_Write &= ~(0x1e);
    Dev_Addr_Byte_Write |= ((Dev_Addr << 1) & 0x1e);

    I2CStart_INA219();
    I2CSendByte_INA219(Dev_Addr_Byte_Write); //10000100,

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    TestAck = WaitAck_INA219();

    //////////////////////////////////////word address///////////////////

    Master_Address_INA219(Reg_Addr);

    GPIO_Setting_IIC_Master_Send_INA219_SDA_OUT();
    SomeNOP();
    I2CStop_INA219();



    //////////////////////////////////////read data,bus voltage///////////////////
    //////////////////////////////////////READ data///////////////////


    Dev_Addr_Byte_Read &= ~(0x1e);
    Dev_Addr_Byte_Read |= ((Dev_Addr << 1) & 0x1e);


    I2CStart_INA219();
    I2CSendByte_INA219(Dev_Addr_Byte_Read); //10000101,

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    TestAck = WaitAck_INA219();

    for(i=0;i<2;i++)
    {
        if(i!=1)
        {
            Data_Rx[i]=Master_Receive_Data_ACK_INA219();
        }
        else
        {
            Data_Rx[i]=Master_Receive_Data_NOTACK_INA219();
        }
    }

    ///////////////////////
    I2CStop_INA219();

    Reg_Value |= (u32)Data_Rx[1];
    Reg_Value |= (u32)Data_Rx[0] << 8;

    return Reg_Value;
}





static void INA219_bus_voltage_normal_read_Dev_Addr(unsigned char Dev_Addr,unsigned char Reg_Addr)
{
    unsigned char i;
//    unsigned char count=10;
//    unsigned char *buff;
    unsigned char TestAck,Data_Rx[16];
//    unsigned int errtime=3000;
    u32 Reg_Value = 0;
    u32 Bus_Voltage = 0;

    unsigned char Dev_Addr_Byte_Write = 0x80;
    unsigned char Dev_Addr_Byte_Read = 0x81;

    //GPIO_Setting_IIC_General();
    gpio_port_1_cfg(9,OUT,0); // GPIO 25 - GPIO,SDA - OUT,IN
    gpio_port_1_cfg(10,OUT,0); // GPIO 26 - GPIO,SCL - OUT
    //////////////////////////////////////device address///////////////////

    //Master Send
    GPIO_Setting_IIC_Master_Send_INA219();
    SomeNOP();

    Dev_Addr_Byte_Write &= ~(0x1e);
    Dev_Addr_Byte_Write |= ((Dev_Addr << 1) & 0x1e);

    I2CStart_INA219();
    I2CSendByte_INA219(Dev_Addr_Byte_Write); //10000100,

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    TestAck = WaitAck_INA219();
    /*
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }
    */
    //////////////////////////////////////word address///////////////////

    Master_Address_INA219(Reg_Addr);

    GPIO_Setting_IIC_Master_Send_INA219_SDA_OUT();
    SomeNOP();

    I2CStop_INA219();


    //////////////////////////////////////read data,bus voltage///////////////////
    //////////////////////////////////////READ data///////////////////


    Dev_Addr_Byte_Read &= ~(0x1e);
    Dev_Addr_Byte_Read |= ((Dev_Addr << 1) & 0x1e);


    I2CStart_INA219();
    I2CSendByte_INA219(Dev_Addr_Byte_Read); //10000101,

    //Master Receive
    GPIO_Setting_IIC_Master_Receive_INA219_SDA_IN();
    SomeNOP();

    TestAck = WaitAck_INA219();
    /*
    if(TestAck == 1)
    {
        dprint("IIC Master receives ACK from Slave.\n");
    }
    else
    {
        dprint("IIC Master do not receive ACK from Slave.\n");
        return;
    }
    */


    for(i=0;i<2;i++)
    {
        if(i!=1)
        {
            Data_Rx[i]=Master_Receive_Data_ACK_INA219();
        }
        else
        {
            Data_Rx[i]=Master_Receive_Data_NOTACK_INA219();
        }
    }

    ///////////////////////

    I2CStop_INA219();


    dprint("Device Addr: %d \n",Dev_Addr);
    dprint("bus voltage register value D15-D8:%x,D7-D0:%x \n",Data_Rx[0],Data_Rx[1]);

    Reg_Value |= Data_Rx[1];
    Reg_Value |= Data_Rx[0] << 8;
    Reg_Value = Reg_Value >> 3;
    Bus_Voltage = Reg_Value * 4;

    dprint("Bus Voltage: %d mv\n",Bus_Voltage);
    dprint("\n");
}


//wangyunl,20111013//////////////////////////////////////////////////////////////////////////
static void INA219_Register_Update(u32 Dev_Addr, u32 Reg_Addr, u32 Reg_Val)
{
//    u32 Dev_Addr=0;
//    u32 Reg_Addr=0;
//    u32 Reg_Val=0;
//    u32 Temp=0;
//    u32 Temp_Val=0;
//    u32 i=0;
//    u32 Value_End=0;
    u32 Value=0;
//    u32 PG = 0;

//    dprint("Input Device Address: ");
//    Dev_Addr = asc_getc() - '0';
    dprint("Dev_Addr=%u\n",Dev_Addr);
//
//    dprint("Input Register Address for update:");
//    Reg_Addr = asc_getc() - '0';
    dprint("Reg_Addr=%u\n",Reg_Addr);
//
//    dprint("Input Register Value for update(hex value,end with 'x'):");
//    while(1)
//    {
//
//        Temp = asc_getc();
//
//        dprint("Temp=%c ",Temp);
//
//        if(Temp == 'x')
//        {
//            break;
//        }
//        else if((Temp >= '0') && (Temp <= '9') ){
//            Temp_Val = Temp - '0';
//        }
//        else if((Temp >= 'A') && (Temp <= 'F') ){
//            Temp_Val = Temp - 'A' + 10;
//        }
//        else if((Temp >= 'a') && (Temp <= 'f') ){
//            Temp_Val = Temp - 'a' + 10;
//        }
//        else{
//            dprint("Wrong Input,Try again. \n");
//            dprint("Input Register Value for update(hex value,end with 'x'):");
//            Reg_Val = 0;
//            Temp = 0;
//            Temp_Val = 0;
//        }
//
//        dprint("Temp_Val=0x%x ",Temp_Val);
//        dprint("Reg_Val=0x%x ",Reg_Val);
//        Reg_Val = (Reg_Val * 16)  + Temp_Val ;
//        dprint("Reg_Val=0x%x ,",Reg_Val);
//    }

    dprint("Reg_Val=0x%08x\n",Reg_Val);


    INA219_Register_Write_Dev_Addr_Value(Dev_Addr,Reg_Addr,Reg_Val);


    Value = INA219_Register_Read_Dev_Addr(Dev_Addr,Reg_Addr);
    dprint("Register %d: 0x%x \n",Reg_Addr,Value);

    Cal_Val = Value;

}


static void INA219_bus_voltage_normal_read_individual_chip(unsigned char Dev_Addr, unsigned char Reg_Addr)
{
//    unsigned char Dev_Addr=0;
//    unsigned char Reg_Addr=0;
    u32 Value=0;
    u32 PG = 0;

    dprint("\n");

//    dprint("Input Device Address: ");
//    Dev_Addr = asc_getc() - '0';
    dprint("Dev_Addr=%u\n", (unsigned int)Dev_Addr);
//
//    dprint("Input Register Address:(9 for all register) ");
//    Reg_Addr = asc_getc() - '0';
    dprint("Reg_Addr=%u\n", (unsigned int)Reg_Addr);


    //INA219_Register_Write_Dev_Addr_Value(Dev_Addr,0,Con_Val);
    //INA219_Register_Write_Dev_Addr_Value(Dev_Addr,5,Cal_Val);

    if((Reg_Addr == 0) | (Reg_Addr == 9) )
    {
        Value = INA219_Register_Read_Dev_Addr(Dev_Addr,0);
        dprint("Register 0,configuration : 0x%x \n",Value);
        dprint("RST = 0x%x \n",((Value >>15) & 0x1));
        dprint("BRNG = 0x%x \n",((Value >>13) & 0x1));
        dprint("PG[1:0] = 0x%x \n",((Value >>11) & 0x3));
        dprint("BADC[4:1] = 0x%x \n",((Value >>7) & 0xf));
        dprint("SADC[4:1] = 0x%x \n",((Value >>3) & 0xf));
        dprint("MODE[3:1] = 0x%x \n",((Value >>0) & 0x7));
        dprint("\n");
        PG = (Value >>11) & 0x3;
    }

    if((Reg_Addr == 1) | (Reg_Addr == 9) )
    {

        Value = INA219_Register_Read_Dev_Addr(Dev_Addr,1);
        dprint("Register 1,shunt voltage : 0x%x \n",Value);
        dprint("Sign = 0x%x \n",((Value >>(12+PG)) & (0xf >> PG)));
        dprint("Value = 0x%x \n",((Value >>0) & (0x7fff >> (3-PG))));
        dprint("\n");
    }

    if((Reg_Addr == 2) | (Reg_Addr == 9) )
    {

        Value = INA219_Register_Read_Dev_Addr(Dev_Addr,2);
        dprint("Register 2,bus voltage : 0x%x \n",Value);
        dprint("Bus Voltage register value = 0x%x \n",((Value >>3) & 0x1fff));
        dprint("CNVR = 0x%x \n",((Value >>1) & 0x1));
        dprint("OVF = 0x%x \n",((Value >>0) & 0x1));
        dprint("Bus Voltage = %d mv\n",((Value >>3)*4 ));
        dprint("\n");
    }

    if((Reg_Addr == 3) | (Reg_Addr == 9) )
    {
        Value = INA219_Register_Read_Dev_Addr(Dev_Addr,3);
        dprint("Register 3,power : 0x%x \n",Value);
        dprint("\n");
    }

    if((Reg_Addr == 4) | (Reg_Addr == 9) )
    {
        Value = INA219_Register_Read_Dev_Addr(Dev_Addr,4);
        dprint("Register 4,current : 0x%x \n",Value);
        dprint("Sign = 0x%x \n",((Value >>15) & 0x1));
        dprint("Value = 0x%x \n",((Value >>0) & (0x7fff )));
        dprint("\n");
    }

    if((Reg_Addr == 5) | (Reg_Addr == 9) )
    {
        Value = INA219_Register_Read_Dev_Addr(Dev_Addr,5);
        dprint("Register 5,calibration : 0x%x \n",Value);
    }

}


static void INA219_bus_voltage_normal_read_all_vol(void)
{

    //check register power on reset values


    dprint("VDC_IN: \n");
    INA219_bus_voltage_normal_read_Dev_Addr(0,2);
    dprint("+3V3_SYS: \n");
    INA219_bus_voltage_normal_read_Dev_Addr(1,2);
    dprint("+2.5V_GPHY: \n");
    INA219_bus_voltage_normal_read_Dev_Addr(2,2);
    dprint("+3V3_1: \n");
    INA219_bus_voltage_normal_read_Dev_Addr(3,2);
    dprint("+5V_1: \n");
    INA219_bus_voltage_normal_read_Dev_Addr(4,2);
    dprint("+5V: \n");
    INA219_bus_voltage_normal_read_Dev_Addr(5,2);
    dprint("+1V5_1V8: \n");
    INA219_bus_voltage_normal_read_Dev_Addr(6,2);
    dprint("+1V: \n");
    INA219_bus_voltage_normal_read_Dev_Addr(7,2);

}

static void INA219_single_chip_all_display(u32 Dev_Addr,u32 Current_LSB,u32 Calibration_Value)
{
    u32 Current;
//    u32 Current_Real;
    u32 Power;
//    u32 Power_Real;
    //u32 Current_LSB = 1;//uA
    u32 Power_LSB = 20 * Current_LSB;//uW
    u32 Current_Sign;
    u32 Current_Value;
    u32 Power_Value;
    INA219_Register_Write_Dev_Addr_Value(Dev_Addr,0,0x3FFF);    //calculation

    INA219_bus_voltage_normal_read_Dev_Addr(Dev_Addr,2);
    INA219_Register_Write_Dev_Addr_Value(Dev_Addr,5,Calibration_Value); //calculation
    Current = INA219_Register_Read_Dev_Addr(Dev_Addr,4);    //current
    Current_Sign = (Current & 0x8000) >> 15;
    if(Current_Sign == 0) //0, positive current
    {
        Current_Value = Current * Current_LSB;
    }
    else
    {
        Current_Value = ((~((Current & 0x7fff) - 1)) & 0x7fff)* Current_LSB ;
    }
    dprint("Current = %d mA (Sign:%d, 0:positive,1:negative) \n",Current_Value/1000,Current_Sign);

    Power = INA219_Register_Read_Dev_Addr(Dev_Addr,3);  //power
    Power_Value = Power * Power_LSB;
    dprint("Power = %d mW \n",Power_Value/1000);
    dprint("========================================= \n");


}



static void INA219_all_display(void)
{

    u32 cal = 36197;

    u32 cal0 = cal;
    u32 cal1 = 38367;       //cal * (452/414.5)=39471, cal * (452/465)  +3v3_sys
    u32 cal2 = cal;
    u32 cal3 = 37909;       //cal * (186/177.6),    +3v3_1
    u32 cal4 = 30050;       //cal * (44/53),        +5V_1
    u32 cal5 = 27910;       //cal * (32/41.5),      +5V
    u32 cal6 = cal;
    u32 cal7 = 38649;       //cal * (822/748)=39778, cal * (822/846)        +1v
    //check register power on reset values


    dprint("VDC_IN: \n");
    //INA219_bus_voltage_normal_read_Dev_Addr(0,2);
    INA219_single_chip_all_display(0,100,cal0);

    dprint("+3V3_SYS: \n");
    //INA219_bus_voltage_normal_read_Dev_Addr(1,2);
    INA219_single_chip_all_display(1,100,cal1);

    dprint("+2.5V_GPHY: \n");
    //INA219_bus_voltage_normal_read_Dev_Addr(2,2);
    INA219_single_chip_all_display(2,100,cal2); //2.5v,10.5 ohm, 91.7 ohm load

    dprint("+3V3_1: \n");
    //INA219_bus_voltage_normal_read_Dev_Addr(3,2);
    INA219_single_chip_all_display(3,100,cal3);

    dprint("+5V_1: \n");
    //INA219_bus_voltage_normal_read_Dev_Addr(4,2);
    INA219_single_chip_all_display(4,100,cal4);

    dprint("+5V: \n");
    //INA219_bus_voltage_normal_read_Dev_Addr(5,2);
    INA219_single_chip_all_display(5,100,cal5);

    dprint("+1V5_1V8: \n");
    //INA219_bus_voltage_normal_read_Dev_Addr(6,2);
    INA219_single_chip_all_display(6,100,cal6);

    dprint("+1V: \n");
    //INA219_bus_voltage_normal_read_Dev_Addr(7,2);
    INA219_single_chip_all_display(7,100,cal7);

}



//////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
void IIC_menu(void)
{
    dprint("\n\t===== IIC(GPIO) TEST MENU =====\n\n");
    dprint("SCL=GPIO11,SDA=GPIO9\n\n");
    dprint("[1]\t IIC EEPROM(ATMEL 24C04) access \n");
    dprint("[2]\t IIC Voltage Monitor(TI INA219), Bus Voltage read,individual chip \n");
    dprint("[3]\t IIC Voltage Monitor(TI INA219), Bus Voltage read,all voltage\n");
    dprint("[4]\t IIC Register Update.\n");
    dprint("[5]\t IIC all display(voltage,current,power).\n");

    dprint("[X]\tExit IIC test\n");
    dprint("\n");
    dprint("Press the corresponding number to run the test:\n");
}



void IIC_test(void)
{
    u8 exit_IIC_prg=0;
    char testch=0;

    while(!exit_IIC_prg)
    {
        IIC_menu();

        testch=asc_getc();
        switch(testch)
        {
            case '1' : IIC_EEPROM_AT24C04_Acess();
                  break;

            case '2' : INA219_bus_voltage_normal_read_individual_chip(0, 0);    //  TODO: parameter
                  break;
            case '3' : INA219_bus_voltage_normal_read_all_vol();
                  break;
            case '4' : INA219_Register_Update(0, 0, 0); //  TODO: parameter
                  break;
            case '5' : INA219_all_display();
                  break;



            case 'x' :
            case 'X' : exit_IIC_prg=1;
                  goto end_IIC;
            //    break;
            default : dprint("The input is not recognised\n");
                  break;
        }
        dprint("\nPress any key to return to IIC menu\n");
        asc_getc();
    }
    end_IIC:
    return;
}
#else

 #ifdef CONFIG_PROC_FS

static int print_help(char *buf)
{
    int len = 0;

    len += sprintf(buf + len, "echo <cmd> [parameter] > /proc/driver/ina219\n");
    len += sprintf(buf + len, "cmd/parameter:\n");
    len += sprintf(buf + len, "  IIC_EEPROM_AT24C04_Acess\n");
    len += sprintf(buf + len, "  INA219_bus_voltage_normal_read_individual_chip <dev_addr> <reg_addr>\n");
    len += sprintf(buf + len, "  INA219_bus_voltage_normal_read_all_vol\n");
    len += sprintf(buf + len, "  INA219_Register_Update <dev_addr> <reg_addr> <reg_val>\n");
    len += sprintf(buf + len, "  INA219_all_display\n");

    return len;
}

static int proc_read_ina219(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    len += print_help(page + len);

    *eof = 1;

    return len;
}

static int proc_write_ina219(struct file *file, const char *buf, unsigned long count, void *data)
{
    char str[2048];
    char *p;
    int len, rlen;

    char *cmds[] = {
        "IIC_EEPROM_AT24C04_Acess",
        "INA219_bus_voltage_normal_read_individual_chip",
        "INA219_bus_voltage_normal_read_all_vol",
        "INA219_Register_Update",
        "INA219_all_display"
    };
    int cmd;
    unsigned int dev_addr = 0, reg_addr = 0, reg_val = 0;

    len = count < sizeof(str) ? count : sizeof(str) - 1;
    rlen = len - copy_from_user(str, buf, len);
    while ( rlen && str[rlen - 1] <= ' ' )
        rlen--;
    str[rlen] = 0;
    for ( p = str; *p && *p <= ' '; p++, rlen-- );
    if ( !*p )
        return count;

    for (  cmd = 0; cmd < NUM_ENTITY(cmds) && strncasecmp(p, cmds[cmd], strlen(cmds[cmd])) != 0; cmd++ );
    switch ( cmd ) {
        case 0:
            IIC_EEPROM_AT24C04_Acess();
            break;
        case 1:
            p += strlen(cmds[cmd]) + 1;
            if ( *p >= '0' && *p <= '9' )
                dev_addr = simple_strtol(p, &p, 0);
            while ( *p && *p <= ' ' )
                p++;
            if ( *p >= '0' && *p <= '9' )
                reg_addr = simple_strtol(p, &p, 0);
            INA219_bus_voltage_normal_read_individual_chip(dev_addr, reg_addr);
            break;
        case 2:
            INA219_bus_voltage_normal_read_all_vol();
            break;
        case 3:
            p += strlen(cmds[cmd]) + 1;
            if ( *p >= '0' && *p <= '9' )
                dev_addr = simple_strtol(p, &p, 0);
            while ( *p && *p <= ' ' )
                p++;
            if ( *p >= '0' && *p <= '9' )
                reg_addr = simple_strtol(p, &p, 0);
            while ( *p && *p <= ' ' )
                p++;
            if ( *p >= '0' && *p <= '9' )
                reg_val = simple_strtol(p, &p, 0);
            INA219_Register_Update(dev_addr, reg_addr, reg_val);
            break;
        case 4:
            INA219_all_display();
            break;
        default:
            print_help(str);
            printk(str);
    }

    return count;
}

static void proc_file_create(void)
{
    struct proc_dir_entry *res;

    res = create_proc_entry("driver/ina219",
                            0775,
                            NULL);
    if ( res ) {
        res->read_proc  = proc_read_ina219;
        res->write_proc = proc_write_ina219;
    }
}

static void proc_file_delete(void)
{
    remove_proc_entry("driver/ina219", NULL);
}

 #else   //  CONFIG_PROC_FS

static void proc_file_create(void)
{
}

static void proc_file_delete(void)
{
}

 #endif  //  CONFIG_PROC_FS

static int __init ina219_init(void)
{
    proc_file_create();

    return 0;
}

static void __exit ina219_exit(void)
{
    proc_file_delete();

    return;

}

module_init(ina219_init);
module_exit(ina219_exit);

#endif
