#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <stdarg.h>
// #include <unistd.h>

#include <cellstatus.h>
#include <cell/cell_fs.h>
#include <cell/pad.h>

#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <time.h>

// #include "inc/exports.h"
#include "printf.h"

#define THREAD_NAME "EBOOTLoader_thread"
#define STOP_THREAD_NAME "EBOOTLoader_stop_thread"

#define SYSCALL_OPCODE_UNLOAD_VSH_PLUGIN			0x364F
#define SYSCALL8_OPCODE_PS3MAPI						0x7777
#define PS3MAPI_OPCODE_GET_ALL_PROC_PID				0x0021
#define PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID			0x0022
#define PS3MAPI_OPCODE_LOAD_PROC_MODULE				0x0044

#define MYSPRX	"/dev_hdd0/tmp/MySPRX_Menu.sprx"
#define CONFIG_HDD	"/dev_hdd0/sprx.txt"

#define BEEP1 { system_call_3(392, 0x1007, 0xA,   0x6); }
#define BEEP2 { system_call_3(392, 0x1007, 0xA,  0x36); }
#define BEEP3 { system_call_3(392, 0x1007, 0xA, 0x1B6); }

SYS_MODULE_INFO(EBOOTLoader, 0, 1, 0);
SYS_MODULE_START(EBOOTLoader_start);
SYS_MODULE_STOP(EBOOTLoader_stop);

static sys_ppu_thread_t thread_id=-1;
static int done = 0;

int EBOOTLoader_start(uint64_t arg);
int EBOOTLoader_stop(void);

extern int vshtask_A02D46E7(int arg, const char *msg);
#define vshtask_notify(msg) vshtask_A02D46E7(0, msg)
extern uint32_t paf_F21655F3(const char *sprx_name);
#define FindLoadedPlugin paf_F21655F3
extern uint32_t vshmain_EB757101(void);
#define GetCurrentRunningMode vshmain_EB757101
extern FILE *stdc_69C27C12(const char *filename, const char *mode); // fopen()
#define fopen stdc_69C27C12
extern int stdc_E1BD3587(FILE *stream); // fclose()
#define fclose stdc_E1BD3587
extern char *stdc_AF44A615(char *str, int num, FILE * stream); // fgets()
#define fgets stdc_AF44A615
extern size_t stdc_2F45D39C(const char *str);                                         // strlen()
#define strlen stdc_2F45D39C

static inline void _sys_ppu_thread_exit(uint64_t val)
{
	system_call_1(41, val);
}

static inline sys_prx_id_t prx_get_module_id_by_address(void *addr)
{
	system_call_1(461, (uint64_t)(uint32_t)addr);
	return (int)p1;
}

/* static void * getNIDfunc(const char * vsh_module, uint32_t fnid, int32_t offset)
{
	// 0x10000 = ELF
	// 0x10080 = segment 2 start
	// 0x10200 = code start

	uint32_t table = (*(uint32_t*)0x1008C) + 0x984; // vsh table address
//	uint32_t table = (*(uint32_t*)0x1002C) + 0x214 - 0x10000; // vsh table address
//	uint32_t table = 0x63A9D4;


	while(((uint32_t)*(uint32_t*)table) != 0)
	{
		uint32_t* export_stru_ptr = (uint32_t*)*(uint32_t*)table; // ptr to export stub, size 2C, "sys_io" usually... Exports:0000000000635BC0 stru_635BC0:    ExportStub_s <0x1C00, 1, 9, 0x39, 0, 0x2000000, aSys_io, ExportFNIDTable_sys_io, ExportStubTable_sys_io>

		const char* lib_name_ptr =  (const char*)*(uint32_t*)((char*)export_stru_ptr + 0x10);

		if(strncmp(vsh_module, lib_name_ptr, strlen(lib_name_ptr))==0)
		{
			// we got the proper export struct
			uint32_t lib_fnid_ptr = *(uint32_t*)((char*)export_stru_ptr + 0x14);
			uint32_t lib_func_ptr = *(uint32_t*)((char*)export_stru_ptr + 0x18);
			uint16_t count = *(uint16_t*)((char*)export_stru_ptr + 6); // number of exports
			for(int i=0;i<count;i++)
			{
				if(fnid == *(uint32_t*)((char*)lib_fnid_ptr + i*4))
				{
					// take address from OPD
					return (void**)*((uint32_t*)(lib_func_ptr) + i) + offset;
				}
			}
		}
		table += 4;
	}
	return 0;
}

static void show_msg(char* msg)
{
	if(!vshtask_notify)
		vshtask_notify = (void*)((int*)getNIDfunc("vshtask", 0xA02D46E7, 0));

	if(strlen(msg)>200) msg[200]=0;

	if(vshtask_notify)
		vshtask_notify(0, msg);
} */


static uint8_t plugin_running;
enum current_proc {IN_VSH = 1, IN_GAME = 2};

/* static int32_t load_plugin_to_game(uint64_t prx_path, void *arg, uint32_t arg_size)
{
	int32_t r;
	char pid_str[128];
	uint32_t pid_list[16];
	sys_prx_id_t game_pid = 0;

	{system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (uint64_t)((uint32_t)pid_list));}
	for(int i = 0; i < 16; i++)
	{
		if (1 < pid_list[i])
		{
			memset(pid_str, 0, sizeof(pid_str));
			{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (uint64_t)((uint32_t)pid_list[i]), (uint64_t)((uint32_t)pid_str));}
			if (strstr(pid_str, "EBOOT") || strstr(pid_str, "DBZ3.BIN"))
			{
				game_pid = pid_list[i];
				break;
			}
			else if ((strstr(pid_str, "vsh") == NULL) && (strstr(pid_str, "agent") == NULL) && (strstr(pid_str, "sys") == NULL))
			{
				// in case it's a self
				game_pid = pid_list[i];
				break;
			}
		}
	}

	{system_call_6(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LOAD_PROC_MODULE, (uint64_t)game_pid, (uint64_t)((uint32_t)prx_path), (uint64_t)((uint32_t)arg), (uint64_t)((uint32_t)arg_size));
    r = (int32_t)p1;}

	return(r);
} */

static sys_prx_id_t load_start(const char *path)
{
    int modres, res;
    sys_prx_id_t id;
	// id = sys_prx_load_module_on_memcontainer(path, 0x3f000006, 0, NULL);
    id = sys_prx_load_module(path, 0, 0);
    if (id < CELL_OK)
	{
		BEEP3
		return id;
    }
    res = sys_prx_start_module(id, 0, NULL, &modres, 0, NULL);
    if (res < CELL_OK)
	{
		BEEP3
		return res;
    }
	else
		BEEP1
    return id;
}

static int check_for_booted_game(void)
{
	int32_t r;
	if (FindLoadedPlugin("game_plugin") != 0)
	{
		char line[256];
		int len;
		char tmp[256];

		// FILE * f = fopen(CONFIG_USB,"r");
		FILE * f = fopen(CONFIG_HDD,"r");
		if(!f)
		{
			// r = load_plugin_to_game((uint64_t)MYSPRX, 0, 0);
			r = load_start(MYSPRX);
			if (r == 0)
			{
				plugin_running = IN_GAME;
				sprintf((char*)tmp, "PRX loaded\n%s", MYSPRX);
				vshtask_notify(tmp);
				sys_timer_sleep(1);
			}
			else
			{
				sprintf((char*)tmp, "PRX Error\n%s", MYSPRX);
				vshtask_notify(tmp);
				sys_timer_sleep(1);
				goto exit;
			}
		}

		while(fgets(line, sizeof line, f) != NULL)
		{
			len = strlen(line);
			if(line[0] != '/' || len == 0) continue;
			if(line[len-1] == '\n') line[len-1] = 0;
			if(line[len-2] == '\r') line[len-2] = 0;

			// r = load_plugin_to_game((uint64_t)((uint32_t)line), 0, 0);
			r = load_start((char*)line);
			if (r == 0)
			{
				plugin_running = IN_GAME;
				sprintf((char*)tmp, "PRX loaded\nPATH: %s", line);
				vshtask_notify(tmp);
				sys_timer_sleep(1);
			}
			else
			{
				sprintf((char*)tmp, "PRX Error\nPATH: %s", line);
				vshtask_notify(tmp);
				sys_timer_sleep(1);
				goto exit;
			}
		}

		fclose(f);
	}
	return CELL_OK;
exit:
	return (-1);
}

/* static int cobra_mamba_syscall_unload_prx_module(uint32_t slot)
{
	system_call_2(8, SYSCALL_OPCODE_UNLOAD_VSH_PLUGIN, (uint64_t)slot);
	return_to_user_prog(int);
} */

static void EBOOTLoader_thread(uint64_t arg)
{
	sys_timer_sleep(6);

	vshtask_notify("EBOOTLoader started");
	
	while (!done)
	{
/* 		if(vshmain_EB757101() != 0) // if game has booted
		{
			for (int x = 0; x < (5 * 100); x++) //5 second delay
				sys_timer_usleep(10000);
			if(check_for_booted_game())
				vshtask_notify("Game Plugin loaded to Game Process");
		} */
		for (int x = 0; x < (15 * 100); x++) //15 second delay
			sys_timer_usleep(10000);
		CellPadData pdata;
		if (cellPadGetData(0, &pdata) == CELL_PAD_OK && pdata.len > 0)
		{
			if (pdata.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R1)
			{
				if (pdata.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE)
				{				
					if(vshmain_EB757101() != 0) // if game has booted
						check_for_booted_game();
				}
			}
		}
		sys_timer_usleep(500000);
	}
	// cobra_mamba_syscall_unload_prx_module(5);
	sys_ppu_thread_exit(0);
}

int EBOOTLoader_start(uint64_t arg)
{
	sys_ppu_thread_create(&thread_id, EBOOTLoader_thread, 0, 3000, 0x100, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME);
		
	_sys_ppu_thread_exit(0);	
	return SYS_PRX_RESIDENT;
	// return SYS_PRX_NO_RESIDENT;
}

static void EBOOTLoader_stop_thread(uint64_t arg)
{
	done = 1;
	
	if (thread_id != (sys_ppu_thread_t)-1)
	{
		uint64_t exit_code;
		sys_ppu_thread_join(thread_id, &exit_code);
	}
	
	sys_ppu_thread_exit(0);
}

static void finalize_module(void)
{
	uint64_t meminfo[5];
	
	sys_prx_id_t prx = prx_get_module_id_by_address(finalize_module);
	
	meminfo[0] = 0x28;
	meminfo[1] = 2;
	meminfo[3] = 0;
	
	system_call_3(482, prx, 0, (uint64_t)(uint32_t)meminfo);		
}

int EBOOTLoader_stop(void)
{
	sys_ppu_thread_t t;
	uint64_t exit_code;
	
	sys_ppu_thread_create(&t, EBOOTLoader_stop_thread, 0, 0, 0x100, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
	sys_ppu_thread_join(t, &exit_code);	
	
	finalize_module();
	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}
