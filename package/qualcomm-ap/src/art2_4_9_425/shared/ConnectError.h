


#ifdef _WINDOWS
	#ifdef PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif



enum
{
    ConnectNartTrying=5000,
    ConnectNart,
    ConnectNartBad,
    ConnectNartTimeout,
    ConnectNartTest,    
    ConnectNartRead,  //5005
    ConnectNartWrite,
    ConnectNartClose,
    ConnectNartCommand,
    ConnectNartResponse, 
    ConnectNartError, //5010
    ConnectNartDone,
    ConnectNartIndexBad,
    ConnectNartNoConnection,
	ConnectNartData, 

    ConnectGuiListen=5100,
    ConnectGuiListenBad,
    ConnectGuiTrying,
    ConnectGui,
    ConnectGuiBad,
    ConnectGuiRead, //5105
    ConnectGuiWrite,
    ConnectGuiClose,
    ConnectGuiWait,
	ConnectGuiAccept,
};

#define ConnectNartTryingFormat "Trying to connect to nart[%d] on %s:%d."
#define ConnectNartFormat "Connected to nart[%d] on %s:%d."
#define ConnectNartBadFormat "Can't connect to nart[%d] on %s:%d."
#define ConnectNartTimeoutFormat "No response from nart[%d]."
#define ConnectNartTestFormat "Good link to nart[%d]."
#define ConnectNartReadFormat "Read error from nart[%d]."
#define ConnectNartWriteFormat "Write error to nart[%d]."
#define ConnectNartCloseFormat "Closed connection to nart[%d]."
#define ConnectNartCommandFormat "Command \"%s\" to nart[%d]."
#define ConnectNartResponseFormat "Response \"%s\" from nart[%d]."
#define ConnectNartErrorFormat "Error \"%s\"from nart[%d]."
#define ConnectNartDoneFormat "Done \"%s\" from nart[%d]."
#define ConnectNartIndexBadFormat "Bad nart[%d]."
#define ConnectNartNoConnectionFormat "No connection to nart[%d]."
#define ConnectGuiListenFormat "Listening for control process connections on %d." 
#define ConnectGuiListenBadFormat "Can't open control process listen port %d." 
#define ConnectGuiTryingFormat "Trying to connect to control process on %s:%d."
#define ConnectGuiFormat "Connected to control process on %s:%d."
#define ConnectGuiBadFormat "Can't connect to control process on %s:%d."
#define ConnectGuiReadFormat "Read error from control process."
#define ConnectGuiWriteFormat "Write error to control process."
#define ConnectGuiCloseFormat "Closed connection to control process."
#define ConnectGuiWaitFormat "Waiting for connection from control process."
#define ConnectNartDataFormat "Data \"%s\" from nart[%d]."
#define ConnectGuiAcceptFormat "Accepted control connection from client %d."

#define ConnectNartTryingFormatInput "Trying to connect to nart[%d] on %[^:]:%d."
#define ConnectNartFormatInput "Connected to nart[%d] on %[^:]:%d."
#define ConnectNartBadFormatInput "Can't connect to nart[%d] on %[^:]:%d."
#define ConnectNartTimeoutFormatInput "No response from nart[%d]."
#define ConnectNartTestFormatInput "Good link to nart[%d]."
#define ConnectNartReadFormatInput "Read error from nart[%d]."
#define ConnectNartWriteFormatInput "Write error to nart[%d]."
#define ConnectNartCloseFormatInput "Closed connection to nart[%d]."
#define ConnectNartCommandFormatInput "Command \"%[^\"]\" to nart[%d]."
#define ConnectNartResponseFormatInput "Response \"%[^\"]\" from nart[%d]."
#define ConnectNartErrorFormatInput "Error \"%[^\"]\" from nart[%d]."
#define ConnectNartDoneFormatInput "Done \"%[^\"]\" from nart[%d]."
#define ConnectNartIndexBadFormatInput "Bad nart[%d]."
#define ConnectNartNoConnectionFormatInput "No connection to nart[%d]."
#define ConnectGuiListenFormatInput "Listening for control process connections on %d." 
#define ConnectGuiListenBadFormatInput "Can't open control process listen port %d." 
#define ConnectGuiTryingFormatInput "Trying to connect to control process on %[^:]:%d."
#define ConnectGuiFormatInput "Connected to control process on %[^:]:%d."
#define ConnectGuiBadFormatInput "Can't connect to control process on %[^:]:%d."
#define ConnectGuiReadFormatInput "Read error from control process."
#define ConnectGuiWriteFormatInput "Write error to control process."
#define ConnectGuiCloseFormatInput "Closed connection to control process."
#define ConnectGuiWaitFormatInput "Waiting for connection from control process."
#define ConnectNartDataFormatInput "Data \"%[^\"]\" from nart[%d]."
#define ConnectGuiAcceptFormatInput ConnectGuiAcceptFormat


extern PARSEDLLSPEC void ConnectErrorInit(void);

