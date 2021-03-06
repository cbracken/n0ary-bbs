#include <stdio.h>

extern char
	prompt_string[4096],
	*Bbs_Call,
	*Bbs_MyCall,
	*Bbs_FwdCall,
	*Bbs_Sysop,
	*Bbs_Dir,
	*Bbs_Hloc,
	*Bbs_Domain,
	*Bbs_Header_Comment,
	*Bbs_Motd_File,
	*Bbs_Translate_File,
	*Bbs_NoSub_File,
	*Bbs_History_File,
	*Bbs_History_Path,
	*Bbs_WxLog_File,
	*Bbs_WxData_File,
	*Bbs_WxOutdoor_File,
	*Bbs_WxOutdoorYest_File,
	*Bbs_WxIndoor_File,
	*Bbs_WxIndoorYest_File,
	*Bbs_Sign_Path,
	*Bbs_Callbk_Path,
	*Bbs_Info_Path,
	*Bbs_Distrib_Path,
	*Bbs_Event_Path,
	*Bbs_Event_Dir,
	*Bbs_Vacation_Path,
	*Bbs_FileSys_Path,
	*Bbs_Sysop_Passwd,
	*Bbs_HelpMsg_Index,
	*Bbs_HelpMsg_Data,
	*Bbs_Hloc_Script,
	*Bbs_Personal_File,
	*Bbs_Reject_File,
	*Bbs_Log_Path,
	*Msgd_System_File,
	*Via;

extern int
	sock,
	bbsd_sock,
#if 0
	Smtp_Port,
#endif
	Bbs_Import_Size,
	Bbs_Msg_Loop,
	DoCRLFEndings,
        monitor_connected,
        monitor_port,
        monitor_sock,
        monitor_fd,
        escape_tnc_commands;

extern time_t
	inactivity_time,
	start_time;

extern long
	Bbs_Timer_Tnc,
	Bbs_Timer_Phone,
	Bbs_Timer_Console;

extern struct ConfigurationList ConfigList[];

extern FILE *logfile;

extern struct RemoteAddr RemoteAddr;
