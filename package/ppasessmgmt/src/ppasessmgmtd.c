#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/autoconf.h>
#include "ppasessmgmt.h"

#undef CONFIG_IFX_PMCU
#include <net/ifx_ppa_api.h>

#define PID_FILE                "/var/run/ppasessmgmtd.pid"
//#define PID_FILE                "/tmp/ppasessmgmtd.pid"

int socket_fd;
#define DELETE_LIST 0
#define ADD_LIST 1

#define uint32_t          unsigned int

typedef struct sess_node {
	uint32_t session;
	uint32_t wt_flag;
} session_node;

typedef struct hist_node {
	uint32_t session;
	struct hist_node *next;
} history_node;

session_node **add_list=NULL, **del_list=NULL;
history_node **history_list=NULL;
int num_history=0;

/*##############################################################*
 * redirct prints to /dev/console
 *#############################################################*/
int iprintf(const char* sFmt, ...)
{
	va_list		vlVars;
	char		sBuf[512];
	int			iRet;
	FILE*		fnOut;

//	if ( (gEnableDebug == 0) || (sFmt == ARC_COM_NULL) )
//		return 0;
		
#if defined(__x86_64__) || defined(__i386__)
	fnOut = fopen( "/dev/tty", "w" );
#else
	fnOut = fopen( "/dev/console", "w" );
#endif
	if (fnOut == NULL)
		return 0;

	sBuf[sizeof(sBuf)-1] = '\0';

	va_start(vlVars,sFmt);

	iRet = vsnprintf(sBuf,sizeof(sBuf),sFmt,vlVars);

	va_end(vlVars);

	if (sBuf[sizeof(sBuf)-1] != '\0')
	{
		fprintf( fnOut, "NOTE: my_printf() overflow!!!\n");
	}

	sBuf[sizeof(sBuf)-1] = '\0';

	fprintf( fnOut, "%s", sBuf );
	fflush( fnOut );
	fclose( fnOut );

	return iRet;
}

/*##############################################################*
 * prints the current status of the ppa session management
 * deamon
 *#############################################################*/
static void print_ppasessmgmtd_status(mgmt_args param_vals)
{
	iprintf(" Polling Interval = %d\n", param_vals.interval);
	iprintf(" Priority Threshold = %d\n", param_vals.priority);
	iprintf(" Byte Rate based Management = %d\n", param_vals.byteratebased);
	iprintf(" Packet	Rate based Management =	%d\n", param_vals.packetratebased);
	iprintf(" Poll history =	%d\n",	param_vals.history);
	iprintf(" Maximum delete	sessions = %d\n", param_vals.maxdelsession);
	iprintf(" Mode =	%d\n",	param_vals.mode);
}

static int do_ioctl_cmd(int ioctl_cmd, void *data)
{
	int ret = 0;
	int fd  = 0;
	if ((fd = open (PPA_DEVICE, O_RDWR)) < 0) {
		iprintf ("\n [%s] : open PPA device (%s) failed. (errno=%d)\n", __FUNCTION__, PPA_DEVICE, errno);
		ret = 1;
	} else {
		if ((ret=ioctl (fd, ioctl_cmd, data)) < 0) {
			iprintf ("\n [%s] : ioctl failed. (errno=%d)\n", __FUNCTION__, errno);
			close (fd);
			ret = 1;
		}
		close (fd);
	}
	return ret;
}

session_node *allocate_sess_node(uint32_t session_id, uint32_t flag)
{
	session_node *node=NULL;
	node = (session_node*) malloc(sizeof(session_node));
	if(node!=NULL) {
		memset(node,0,sizeof(session_node));
		node->wt_flag=flag;
		node->session=session_id;
	}
	return node;
}

history_node *allocate_history_node(uint32_t session_id)
{
	history_node *node=NULL;
	node = (history_node*) malloc(sizeof(history_node));
	if(node!=NULL) {
		memset(node,0,sizeof(history_node));
		node->session = session_id;
		node->next = NULL;
	}
	return node;
}

void free_session_list(session_node **list,int len)
{
	int i;
#ifdef DBG_PRINT
	iprintf("free_session_list\n");
#endif
	for(i=0; i<len; i++) {
		if(*(list+i) !=NULL) {
			free(*(list+i));
		} else {
			break;
		}
	}
	bzero(list, sizeof(session_node*)*len);
#ifdef DBG_PRINT
        iprintf("free_session_list return\n");
#endif
}

void free_history_list(history_node *list)
{
	history_node *tmp = list;
	while(list) {
		list=tmp->next;
		if(tmp!=NULL) {
			free(tmp);
		}
		tmp=list;
	}
}

void free_history_tab()
{
	int i;
	for(i=0; i<num_history+1; i++) {
		free_history_list(*(history_list+i));
	}
	free(history_list);
}

/*##############################################################*
 * function checks whether the session passed is already
 * there in the history list
 *##############################################################*/
int in_history_list(uint32_t sessionid)
{
	int i;
	history_node *lst_node=NULL;
	// history list will be maintained as 2 * number of histry to be maintained +1
	// history list[0] will be the newly created list while each iteration
	// if currently we are processing lan sessions
	// history list[2,4,6...] will be lan_sessions
	// and history list[1,3,5...] will be wan_sessions
	for(i=2; i<num_history+1; i+=2) {
		lst_node = *(history_list+i);
		while(lst_node) {
			if(lst_node->session==sessionid) {
#ifdef DBG_PRINT
	iprintf("match in history list\n");
#endif
				return 1;
			} else {
				lst_node=lst_node->next;
			}
		}
	}
	return 0;
}

/*##############################################################*
 * inserts nodes to delete list or add list based on the entry
 * criteria.
 *##############################################################*/
int insert_to_list( session_node **list, PPA_CMD_SESSION_ENTRY* entry,
                    int *num, int max_num, mgmt_args *param_val, int mode)
{
	session_node *new_node = NULL;
	int i, done=0;
	uint32_t wt_flag=0;

//#ifdef DBG_PRINT
//	iprintf("insert_to_list\n");
//#endif

	if( param_val->byteratebased) {
//  packetratebased not supported
//		if(param_val->packetratebased) {
//			wt_flag |= (entry-> prev_sess_bytes + entry-> prev_sess_pkts);
//		} else {
		if(!mode){
			wt_flag |= ((entry-> hw_bytes - entry-> prev_sess_bytes)/100);
		}else{
			wt_flag |= ((entry-> mips_bytes - entry-> prev_sess_bytes)/100);
		}
#ifdef DBG_PRINT
	iprintf("wt_flag %u\n",wt_flag);
#endif

//		}
//	} else {
//		if(param_val->packetratebased) {
//			wt_flag |=  entry-> prev_sess_pkts;
//		}
	}
	
	if( entry->priority >= param_val->priority) {
		wt_flag |= 0x80000000; // set the msb
#ifdef DBG_PRINT
		 iprintf("wt_flag updated based on priority %u\n",wt_flag);
#endif
	}
	if(*num==0) {
		// list is empty
		// first entry in the list
#ifdef DBG_PRINT
	 iprintf("first entry in the list \n");
#endif
		new_node = allocate_sess_node( entry->session, wt_flag);
		if(new_node!=NULL){
			*list = new_node;
			(*num)++;
#ifdef DBG_PRINT
                iprintf("first entry added in list \n");
#endif
		}
	} else {
		for( i=0; i< *num; i++) {
			if(mode) {
				// add list
//#ifdef DBG_PRINT
//				iprintf(" add list \n");
//#endif
				if( wt_flag > (*(list+i))->wt_flag ) {
					new_node = allocate_sess_node( entry->session, wt_flag);
					if(new_node!=NULL) {
					   if(*num<max_num) {
						(*num)++;
					   } else {
						//delete the last entry
						free(*(list+(max_num-1)));
						*(list+(max_num-1))=NULL;
					   }
					   // move the entire list to create empty space
				 	   memmove(list+i+1,list+i,sizeof(session_node *)*(max_num-i-1));
					   *(list+i)=new_node;
					   done=1;
					}
					break;
				}
			} else {
				// delete list
//#ifdef DBG_PRINT
//				 iprintf(" delete list\n");
//#endif
				if( wt_flag < (*(list+i))->wt_flag ) {
					new_node = allocate_sess_node( entry->session, wt_flag);
					if(new_node!=NULL) {
					    if(*num<max_num) {
						(*num)++;
					    } else {
						//delete the last entry
						free(*(list+(max_num-1)));
						*(list+(max_num-1))=NULL;
					    }
					    // move the entire list to create empty space
					    memmove(list+i+1,list+i,sizeof(session_node *)*(max_num-i-1));
					    *(list+i)=new_node;
					    done=1;
					}
					break;
				}
			}
		}
		// reached the end of list; still not inserted & empty spaces in the list
		// insert at the end of list
		if(i<max_num && !done) {
			new_node = allocate_sess_node( entry->session, wt_flag);
			if(new_node!=NULL) {
				*(list+i)=new_node;
				(*num)++;
			}
		}
	}
	return 0;
}

/*##############################################################*
* performs the session management function
* #############################################################*/
int do_session_mgmt( uint32_t function, int num_entries, 
		     int num_hw_entries, int max_hw_entries, 
		     mgmt_args *param_val)
{
	uint32_t size=0, hash_index=0;
	int res = 0, i=0;
        PPA_CMD_DATA cmd_info;	
	PPA_CMD_SESSION_EXTRA_ENTRY psession;
	history_node *new_hnode=NULL;
	int num_del_sessions =0, num_add_sessions =0;
	PPA_CMD_SESSIONS_INFO *psession_buffer=NULL;
#ifdef DBG_PRINT
	iprintf("do_session_mgmt\n");
#endif
	for(hash_index=0; hash_index<SESSION_LIST_HASH_TABLE_SIZE; hash_index++)
   	{

		memset( &cmd_info, 0, sizeof(cmd_info) );
	        cmd_info.count_info.flag = 0;
    		cmd_info.count_info.stamp_flag = 0;
    		cmd_info.count_info.hash_index = hash_index+1;

		if(function == PPA_CMD_GET_LAN_SESSIONS )
    		{
		        if( do_ioctl_cmd(PPA_CMD_GET_COUNT_LAN_SESSION, &cmd_info ) != 0 )
            		return 1;
    		} else if( function == PPA_CMD_GET_WAN_SESSIONS )
    		{
        		if( do_ioctl_cmd(PPA_CMD_GET_COUNT_WAN_SESSION, &cmd_info ) != 0 )
            		return 1;
    		}


		if( cmd_info.count_info.count != 0 ) {

			size = sizeof(PPA_CMD_SESSIONS_INFO) + (sizeof(PPA_CMD_SESSION_ENTRY) * ( cmd_info.count_info.count + 1 ));

#ifdef DBG_PRINT
		        iprintf("do_session_mgmt count=%u size:%u\n", cmd_info.count_info.count, size);
#endif
			psession_buffer = (PPA_CMD_SESSIONS_INFO *) malloc ( size );
#ifdef DBG_PRINT
		        iprintf("do_session_mgmt malloc done\n");
#endif
			if( psession_buffer == NULL ) {
				iprintf("do_session_mgmt malloc failed\n");
				return 1;
			}
#ifdef DBG_PRINT
			iprintf("do_session_mgmt malloc succeeded\n");
#endif
    			memset( psession_buffer, 0, size);
			psession_buffer->count_info.count = cmd_info.count_info.count;
		        //set the SESSION_BYTE_STAMPING_FLAG
			psession_buffer->count_info.flag = 0;
			psession_buffer->count_info.stamp_flag = SESSION_BYTE_STAMPING; 
			psession_buffer->count_info.hash_index = hash_index+1;
#ifdef DBG_PRINT
		        iprintf("do_session_mgmt %u\n",psession_buffer->count_info.count);
#endif

			//get session information
			if( (res = do_ioctl_cmd(function, psession_buffer ) != 0 ) ) {
				free( psession_buffer );
				iprintf("do_session_mgmt ioctl failed\n");
				return res;
			}

#ifdef DBG_PRINT
			iprintf("sessions read successfully %u\n", psession_buffer->count_info.count);
#endif
			if(psession_buffer->count_info.count >0) { 
	    			for(i = 0; i < psession_buffer->count_info.count; i++ ) {
					if( psession_buffer->session_list[i].flags & SESSION_ADDED_IN_HW) {
					//session currently in HW
#ifdef DBG_PRINT
						iprintf("session currently in HW.. trying to add to delete list\n");
#endif
						insert_to_list( del_list, &(psession_buffer->session_list[i]),
			                		&num_del_sessions, param_val->maxdelsession, param_val, DELETE_LIST);
					} else {
					//session currently in SW
#ifdef DBG_PRINT
						iprintf("session currently in SW.. trying to add to  add list\n");
#endif
						if( psession_buffer->session_list[i].flags&SESSION_CAN_NOT_ACCEL) {
#ifdef DBG_PRINT
                                                        iprintf("session flagged SESSION_CAN_NOT_ACCEL!\n");
#endif
                                                        psession_buffer->session_list[i].flags &= ~SESSION_CAN_NOT_ACCEL;
						}
						insert_to_list( add_list, &(psession_buffer->session_list[i]),
				               		 &num_add_sessions, param_val->maxdelsession * 2, param_val, ADD_LIST);
					}
				}	
					
				
				if(!param_val->mode) { // non aggressive mode
					if( (num_add_sessions>=param_val->maxdelsession * 2) && (num_del_sessions>=param_val->maxdelsession)) {
						if((*(add_list+num_add_sessions-1))->wt_flag > (*(del_list+num_del_sessions-1))->wt_flag) {
#ifdef DBG_PRINT
						iprintf("found max__delete_sessions * 2 sessions with weight > largest in the del list\n");
#endif
						free(psession_buffer);
						break; // found max__delete_sessions * 2 sessions with weight > largest in the del list
						}
					}
				}
			}
			free(psession_buffer);
	    	}
	}

	if(num_del_sessions!=0 && num_add_sessions!=0) {
		if(param_val->mode){
#ifdef DBG_PRINT
		 iprintf("Aggressive mode \n");
#endif
			 while( num_del_sessions>=1 && num_add_sessions>=1 && (*(add_list+num_add_sessions-1)!=NULL && 
				*(del_list+num_del_sessions-1)!=NULL )&& 
				((*(add_list+num_add_sessions-1))->wt_flag <= (*(del_list+num_del_sessions-1))->wt_flag)){
			 		num_del_sessions--;
		 	}
		}
#ifdef DBG_PRINT
		iprintf("sessions inserted to the lists successfully num_add_sessions=%d num_del_sessions=%d\n",num_add_sessions,num_del_sessions);
		iprintf("max_hw_entries=%d num_hw_entries=%d\n",max_hw_entries,num_hw_entries);
#endif
		//maintain history list :delete the last entry
	    	free_history_list(*(history_list+num_history));
	    	memmove(history_list+1,history_list,sizeof(history_node *)*(num_history));
	    	*history_list=NULL;

	    	if(num_del_sessions>num_add_sessions) {
			num_del_sessions=num_add_sessions / 2;
	    	}

	    	if(num_hw_entries < max_hw_entries) {
			num_del_sessions -= (max_hw_entries - num_hw_entries);
	    	}

		// delete sessions from the delete list
#ifdef DBG_PRINT
		iprintf("session delete started num_add_sessions=%d num_del_sessions=%d\n",num_add_sessions,num_del_sessions);
#endif
	    	if(num_del_sessions > 0) {
			for(i=0; (i<num_del_sessions) && (i<param_val->maxdelsession); i++) {
			//remove session from HW & delete one entry from the list
	      			if(*(del_list+i)!=NULL){
					bzero(&psession, sizeof(PPA_CMD_SESSION_EXTRA_ENTRY));
					psession.flags |= PPA_F_ACCEL_MODE;
					psession.session_extra.accel_enable = 0;
					psession.session = (*(del_list+i))->session;
					psession.lan_wan_flags = SESSION_WAN_ENTRY | SESSION_LAN_ENTRY;
#ifdef DBG_PRINT
					iprintf("trying to delete session %x with weight %u from hardware\n", (*(del_list+i))->session, (*(del_list+i))->wt_flag);
#endif
					if(do_ioctl_cmd(PPA_CMD_MODIFY_SESSION, &psession)==0){
						syslog(LOG_DEBUG,"ppa session %x with weight %u removed from hardware\n", (*(del_list+i))->session, (*(del_list+i))->wt_flag);
					}
#ifdef DBG_PRINT
   				else {
					iprintf("delete session %x with weight %u from hardware FAILED\n", (*(del_list+i))->session, (*(del_list+i))->wt_flag);
				}
#endif
	      			}
	    		}
		}
	
#ifdef DBG_PRINT
	iprintf("session add started num_add_sessions=%d num_del_sessions=%d\n",num_add_sessions,num_del_sessions);
#endif
	    	for(i=0;i<num_add_sessions; i++) {
	        	if(*(add_list+i)!=NULL) {	
		    		if (!(in_history_list((*(add_list+i))->session))) {
					bzero(&psession, sizeof(PPA_CMD_SESSION_EXTRA_ENTRY));
					psession.flags |= PPA_F_ACCEL_MODE;
					psession.session_extra.accel_enable = 1;
					psession.session=(*(add_list+i))->session;
					psession.lan_wan_flags = SESSION_WAN_ENTRY | SESSION_LAN_ENTRY;
#ifdef DBG_PRINT
					iprintf("trying to add session %x with weight %u to  hardware\n", (*(add_list+i))->session, (*(add_list+i))->wt_flag);
#endif
					if(do_ioctl_cmd(PPA_CMD_MODIFY_SESSION, &psession)==0) {
						syslog(LOG_DEBUG,"ppa session %x with weight %u added to hardware\n", (*(add_list+i))->session, (*(add_list+i))->wt_flag);
						//add to history list
						new_hnode=allocate_history_node((*(add_list+i))->session);
			        		if(new_hnode!=NULL) {
				    			if(*history_list==NULL) {
								*history_list=new_hnode;
				    			} else {
								new_hnode->next=(*history_list)->next;
								(*history_list)->next=new_hnode;
				    			}
						}
					}
#ifdef DBG_PRINT
					else {
						iprintf("adding session %x with weight %u to  hardware FAILED\n", (*(add_list+i))->session,(*(add_list+i))->wt_flag);
			     		}
#endif
		    		}
	        	}
	    	}
        }
#ifdef DBG_PRINT
		iprintf("free up all the memory\n");
#endif
		//free up all the memory
	    	free_session_list(add_list,param_val->maxdelsession*2);
	    	free_session_list(del_list,param_val->maxdelsession);
#ifdef DBG_PRINT
	iprintf("return from do_session_management\n");
#endif
	return 0;
}

void common_sighandler(int iSigNum)
{
	switch (iSigNum) {
	case SIGTERM:
		puts("SIGTERM\n");
		close(socket_fd);
		remove(PID_FILE);
		remove(PPASESSMGMT_SOCKET);
		syslog(LOG_NOTICE,"ppasession management daemon killed!!!\n");
		exit(SIGTERM);
		break;
	default:
		iprintf("%s:%d default case %d\n", __func__, __LINE__, iSigNum);
		break;
	}
	return;
}

int main(int argc, char *argv[])
{
	int iRet = 0;
	struct sigaction sa;
	pid_t tPID, sid;
	FILE *pidf = NULL;
	int fd;
	pid_t oldpid = -1;
	char  procpath[64];
	int   retval;
	struct sockaddr_un address;
	int connection_fd;
	socklen_t address_length=0;
	fd_set rfds;
	struct timespec tv;
	pidf = fopen(PID_FILE, "a+");
	if(pidf == NULL) {
		iprintf("%s:%d Error %s\n", __func__, __LINE__, PID_FILE);
		exit(EXIT_FAILURE);
	}
	/* Check for duplicate instance.. */
	fscanf(pidf, "%d", &oldpid);
	snprintf(procpath, sizeof(procpath), "/proc/%d/cmdline", oldpid);
	fd = open(procpath, O_RDONLY);
	if(fd >= 0) {
		if(read(fd, procpath, sizeof(procpath)) > 0) {
			iprintf("(%s)%d is already running\n", procpath, oldpid);
			fclose(pidf);
			exit(1);
		}
	}
	rewind(pidf);
	/* Fork off the parent process */
	tPID = fork();
	if(tPID < 0) {
		exit(EXIT_FAILURE);
	}
	/* If we got a good PID, then we can exit the parent process. */
	if(tPID > 0) {
		exit(EXIT_SUCCESS);
	}
	/* Change the file mode mask */
	/* umask(0); */
	/* Open any logs here */
	/* Create a new SID for the child process */
	sid = setsid();
	if(sid < 0) {
		/* Log the failure */
		exit(EXIT_FAILURE);
	}
	sa.sa_handler = common_sighandler;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	fprintf(pidf, "%d\n", getpid());
	fclose(pidf);
	// default management parameter values
	mgmt_args param_vals = { 60,7,1,0,1,10,0,0 }, new_param_vals;

	del_list=(session_node **) malloc(sizeof(session_node*)*param_vals.maxdelsession);
	if(del_list) 
		bzero(del_list, sizeof(session_node*)*param_vals.maxdelsession);
	else return 1;
	add_list=(session_node **) malloc(sizeof(session_node*)*param_vals.maxdelsession*2);
	if(add_list)
		bzero(add_list, sizeof(session_node*)*param_vals.maxdelsession*2);
	else return 1;
	num_history=param_vals.history*2;
	history_list=(history_node **) malloc(sizeof(history_node *)*(num_history+1));
	if(history_list)
		bzero(history_list, sizeof(history_node *)*(num_history+1));
	else return 1;

	PPA_CMD_MAX_ENTRY_INFO max_entries;
	PPA_CMD_DATA num_entries, num_hw_entries;
	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		iprintf("socket() failed\n");
		return 1;
	}
	unlink(PPASESSMGMT_SOCKET);
	/* start with a clean address structure */
	memset(&address, 0, sizeof(struct sockaddr_un));
	address_length=sizeof(struct sockaddr_un);
	address.sun_family = AF_UNIX;
	snprintf(address.sun_path, UNIX_PATH_MAX, PPASESSMGMT_SOCKET);
	if(bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) {
		iprintf("bind() failed\n");
		return 1;
	}
	if(listen(socket_fd, 5) != 0) {
		iprintf("listen() failed\n");
		return 1;
	}
	bzero(&max_entries,sizeof(max_entries));
	if( do_ioctl_cmd(PPA_CMD_GET_MAX_ENTRY, &max_entries) != 0 ) {
		return 1;
	}

	syslog(LOG_NOTICE,"ppasession management daemon started!\n");

	while(1) {
		connection_fd = 0;
		retval = 0;
		FD_ZERO(&rfds);
		FD_SET(socket_fd, &rfds);
		bzero(&tv, sizeof(struct timespec));
		tv.tv_sec = param_vals.interval;
#ifdef DBG_PRINT
		iprintf("select started\n");
#endif
		retval = pselect(socket_fd+1, &rfds, NULL, NULL, &tv, NULL);
#ifdef DBG_PRINT
		iprintf("select returned...%d \n",retval);
#endif
		if (retval == -1)
			perror("select()");
		else if (retval) {
#ifdef DBG_PRINT
			iprintf("Data is available now.\n");
#endif
			bzero(&new_param_vals, sizeof(mgmt_args));
			/* FD_ISSET(0, &rfds) will be true. */
			if(FD_ISSET(socket_fd, &rfds)) {
				connection_fd = accept(socket_fd, (struct sockaddr *) &address, &address_length);
				if(connection_fd > 0) {
					retval = read(connection_fd, &new_param_vals, sizeof(mgmt_args));
					if(retval > 0) {
						// update the values
						if(new_param_vals.f_stat) {
							param_vals.status=0;
							print_ppasessmgmtd_status(param_vals);
							continue;
						}
						if(new_param_vals.f_inte) {
							param_vals.interval=new_param_vals.interval;
						}
						if(new_param_vals.f_prio) {
							param_vals.priority=new_param_vals.priority;
						}
						if(new_param_vals.f_brate) {
							param_vals.byteratebased=new_param_vals.byteratebased;
						}
						if(new_param_vals.f_prate) {
							param_vals.packetratebased=new_param_vals.packetratebased;
						}
						if(new_param_vals.f_hist) {
							param_vals.history=new_param_vals.history;
						}
						if(new_param_vals.f_maxd ) {
							param_vals.maxdelsession=new_param_vals.maxdelsession;
						}
						if(new_param_vals.f_mode) {
							param_vals.mode=new_param_vals.mode;
						}
						//allocate for dellist, addlist and hist_tab
#ifdef DBG_PRINT
			                        iprintf("allocate for dellist, addlist and hist_tab\n");
#endif
						del_list=(session_node **) realloc(del_list,sizeof(session_node*)*param_vals.maxdelsession);
						if(del_list)
							bzero(del_list, sizeof(session_node*)*param_vals.maxdelsession);
						else break;
						add_list=(session_node **) realloc(add_list,sizeof(session_node*)*param_vals.maxdelsession*2);
						if(add_list)
							bzero(add_list, sizeof(session_node*)*param_vals.maxdelsession*2);
						else break;
						num_history=param_vals.history*2;
						history_list=(history_node **) realloc(history_list,sizeof(history_node *)*(num_history+1));
						if(history_list)
							bzero(history_list, sizeof(history_node *)*(num_history+1));
						else break;
#ifdef DBG_PRINT
						iprintf("-------------%d bytes received---------------\n",retval);
						print_ppasessmgmtd_status(param_vals);
#endif
					} else {
						perror("read()");
						break;
					}
					close(connection_fd);
				} else {
					perror("accept()");
					break;
				}
			}
		}
		// once the timer expires do the session management
		//wan session management
#ifdef DBG_PRINT
		iprintf("wan session management\n");
#endif
		bzero(&num_entries,sizeof(num_entries));
		if( do_ioctl_cmd( PPA_CMD_GET_COUNT_WAN_SESSION, &num_entries) != 0 ) {
#ifdef DBG_PRINT
                iprintf("PPA_CMD_GET_COUNT_WAN_SESSION returned failure");
#endif
			syslog(LOG_NOTICE,"PPA_CMD_GET_COUNT_WAN_SESSION returned 1\n");
		}
		// only if the total sessions are more than maximum allowed hardware sessions
#ifdef DBG_PRINT
                iprintf("wan num_entries:%d\n", num_entries.count_info.count);
#endif

		bzero(&num_hw_entries,sizeof(num_hw_entries));
		num_hw_entries.count_info.flag = SESSION_ADDED_IN_HW;
		if( do_ioctl_cmd( PPA_CMD_GET_COUNT_WAN_SESSION, &num_hw_entries) != 0 ) {
#ifdef DBG_PRINT
                iprintf("PPA_CMD_GET_COUNT_WAN_SESSION returned failure");
#endif
			syslog(LOG_NOTICE,"PPA_CMD_GET_COUNT_WAN_SESSION returned 1\n");
		}
#ifdef DBG_PRINT		
                iprintf("wan num_hw_entries:%d\n", num_hw_entries.count_info.count);
#endif

		if( (num_entries.count_info.count > max_entries.entries.max_wan_entries) ||
				(num_hw_entries.count_info.count < num_entries.count_info.count)) {
			if(do_session_mgmt( PPA_CMD_GET_WAN_SESSIONS, num_entries.count_info.count, num_hw_entries.count_info.count, 
						 max_entries.entries.max_wan_entries, &param_vals)) {
#ifdef DBG_PRINT
                iprintf("do_session_mgmt returned failure");
#endif
			 syslog(LOG_NOTICE,"do_session_mgmt returned 1\n");
			}
		}


		//lan session management
#ifdef DBG_PRINT
		iprintf("lan session management\n");
#endif
		bzero(&num_entries,sizeof(num_entries));
		if( do_ioctl_cmd( PPA_CMD_GET_COUNT_LAN_SESSION, &num_entries) != 0 ) {
#ifdef DBG_PRINT
                iprintf("PPA_CMD_GET_COUNT_LAN_SESSION returned failure");
#endif
			syslog(LOG_NOTICE,"PPA_CMD_GET_COUNT_LAN_SESSION returned 1\n");
		}

		bzero(&num_hw_entries,sizeof(num_hw_entries));
		num_hw_entries.count_info.flag = SESSION_ADDED_IN_HW;
		if( do_ioctl_cmd( PPA_CMD_GET_COUNT_LAN_SESSION, &num_hw_entries) != 0 ) {
#ifdef DBG_PRINT
                iprintf("PPA_CMD_GET_COUNT_LAN_SESSION returned failure");
#endif
			syslog(LOG_NOTICE,"PPA_CMD_GET_COUNT_LAN_SESSION returned 1\n");
		}
#ifdef DBG_PRINT
                iprintf("lan num_hw_entries:%d\n", num_hw_entries.count_info.count);
#endif
		// only if the total sessions are more than maximum allowed hardware sessions
#ifdef DBG_PRINT
		iprintf("lan num_entries:%d max_entries.entries.max_lan_entries=%d\n", num_entries.count_info.count, max_entries.entries.max_lan_entries);
#endif
		if( (num_entries.count_info.count > max_entries.entries.max_lan_entries) ||
				(num_hw_entries.count_info.count < num_entries.count_info.count)) {
			if(do_session_mgmt( PPA_CMD_GET_LAN_SESSIONS, num_entries.count_info.count, num_hw_entries.count_info.count, 
						max_entries.entries.max_lan_entries, &param_vals)) {
#ifdef DBG_PRINT
                iprintf("do_session_mgmt returned failure");
#endif
			syslog(LOG_NOTICE,"do_session_mgmt returned 1\n");
			}
		}
#ifdef DBG_PRINT
		iprintf(" Timer expired ..\n");
#endif
	}

	close(socket_fd);
	remove(PID_FILE);
	remove(PPASESSMGMT_SOCKET);
// freeup
#ifdef DBG_PRINT
	iprintf(" ppasessmgmtd exiting..\n");
#endif
	free_history_tab();
	free(del_list);
	free(add_list);
	return iRet;
}
