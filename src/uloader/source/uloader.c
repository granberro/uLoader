/* uLoader- Project based in YAL and usbloader by Hermes */

/*   
	Copyright (C) 2009 Hermes
    Copyright (C) 2009 Kwiirk (YAL)
	Copyright (C) 2009 Waninkoko (usbloader)
	
    Yet Another Loader.  The barely minimum usb loader
    
    Based on SoftChip, which should be based on GeckoOS...

    no video, no input, try to load the wbfs passed as argument or return to menu.
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include <gccore.h>
#include <string.h>
#include <malloc.h>
#include "defs.h"
#include "wdvd.h"
#include <stdlib.h>
#include <unistd.h>

#include "libpng/pngu/pngu.h"
#include "libwbfs/libwbfs.h"
#include "wbfs.h"
#include "usbstorage2.h"

#include <wiiuse/wpad.h>
#include <sdcard/wiisd_io.h>

#include "patchcode.h"
#include "kenobiwii.h"
#include "wdvd.h"
#include "disc.h"

#define CIOS 222

#include <stdarg.h>
#include <stdio.h>

#include "screen.h"
#include <fat.h>
#include <math.h>


#include "defpng.h"
#include "button1.h"
#include "button2.h"
#include "button3.h"
#include "background.h"
#include "icon.h"

#include "files.h"

#include "ehcmodule.h"
#include "dip_plugin.h"
#include "mload.h"



static u32 ios_36[16] ATTRIBUTE_ALIGN(32)=
{
	0, // DI_EmulateCmd
	0,
	0x2022DDAC, // dvd_read_controlling_data
	0x20201010+1, // handle_di_cmd_reentry (thumb)
	0x20200b9c+1, // ios_shared_alloc_aligned (thumb)
	0x20200b70+1, // ios_shared_free (thumb)
	0x20205dc0+1, // ios_memcpy (thumb)
	0x20200048+1, // ios_fatal_di_error (thumb)
	0x20202b4c+1, // ios_doReadHashEncryptedState (thumb)
	0x20203934+1, // ios_printf (thumb)
};

static u32 ios_38[16] ATTRIBUTE_ALIGN(32)=
{
	0, // DI_EmulateCmd
	0,
	0x2022cdac, // dvd_read_controlling_data
	0x20200d38+1, // handle_di_cmd_reentry (thumb)
	0x202008c4+1, // ios_shared_alloc_aligned (thumb)
	0x20200898+1, // ios_shared_free (thumb)
	0x20205b80+1, // ios_memcpy (thumb)
	0x20200048+1, // ios_fatal_di_error (thumb)
	0x20202874+1, // ios_doReadHashEncryptedState (thumb)
	0x2020365c+1, // ios_printf (thumb)
};




u32 patch_datas[8] ATTRIBUTE_ALIGN(32);


int load_ehc_module()
{
int is_ios=0;


	if(mload_module(ehcmodule, size_ehcmodule)<0) return -1;
	usleep(250*1000);
	

	// Test for IOS

	mload_seek(0x20207c84, SEEK_SET);
	mload_read(patch_datas, 4);
	if(patch_datas[0]==0x6e657665) is_ios=38;
	else
		{
		mload_seek(0x20207ff0, SEEK_SET);
		mload_read(patch_datas, 4);
		if(patch_datas[0]==0x6e657665) is_ios=36;
		}

	if(is_ios==36)
		{
		// IOS 36
		memcpy(ios_36, dip_plugin, 4);		// copy the entry_point
		memcpy(dip_plugin, ios_36, 4*10);	// copy the adresses from the array
		
		mload_seek(0x1377E000, SEEK_SET);	// copy dip_plugin in the starlet
		mload_write(dip_plugin,size_dip_plugin);

		// enables DIP plugin
		mload_seek(0x20209040, SEEK_SET);
		mload_write(ios_36, 4);
		 
		}
	if(is_ios==38)
		{
		// IOS 38

		memcpy(ios_38, dip_plugin, 4);	    // copy the entry_point
		memcpy(dip_plugin, ios_38, 4*10);   // copy the adresses from the array
		
		mload_seek(0x1377E000, SEEK_SET);	// copy dip_plugin in the starlet
		mload_write(dip_plugin,size_dip_plugin);

		// enables DIP plugin
		mload_seek(0x20208030, SEEK_SET);
		mload_write(ios_38, 4);

		}

	

return 0;
}

#define USE_MODPLAYER 1

#ifdef USE_MODPLAYER

#include "asndlib.h"
// MOD Player
#include "gcmodplay.h"

MODPlay mod_track;

#include "lotus3_2.h" 

#endif


/*LANGUAGE PATCH - FISHEARS*/
u32 langsel = 0;
char languages[11][22] =
{
{" Default  "},
{" Japanese "},
{" English  "},
{"  German  "},
{"  French  "},
{" Spanish  "},
{" Italian  "},
{"  Dutch   "},
{"S. Chinese"},
{"T. Chinese"},
{"  Korean  "}
};
/*LANGUAGE PATCH - FISHEARS*/


//---------------------------------------------------------------------------------
/* Global definitions */
//---------------------------------------------------------------------------------


unsigned char *buff_cheats;
int len_cheats=0;

int load_cheats(u8 *discid);
void LoadLogo(void);
void DisplayLogo(void);
int load_disc(u8 *discid);

// to read the game conf datas
u8 temp_data[256*1024] ALIGNED(0x20);

// texture of white-noise animation generated here
u32 game_empty[128*64*3] ALIGNED(0x20);

s8 sound_effects[2][2048] ALIGNED(0x20);

//---------------------------------------------------------------------------------
/* language GUI */
//---------------------------------------------------------------------------------

int idioma=0;

char letrero[2][50][64]=
	{
	{"Return", "Configure", "Delete Favorite", "Add Favorite", "Load Game", "Add to Favorites", "Favorites", "Page", "Ok" ,"Discard",
	" Cheats Codes found !!! ", "Apply Codes ", "Skip Codes ", "Format WBFS", "Selected", "You have one WBFS partition", "Are You Sure You Can Format?",
	" Yes ", " No ", "Formatting...","Formatting Disc Successfull","Formatting Disc Failed",
//22 
	"Return", "Add New Game", "Add PNG Bitmap", "Delete Game", "Format Disc", "Return to Wii Menu", "","","","",
//32
	"Are you sure you want delete this?", "Press A to accept or B to cancel", 
// 34
"Insert the game DVD disc...", "ERROR! Aborted", "Press B to Abort","Opening DVD disc...", "ERROR: Not a Wii disc!!",
"ERROR: Game already installed!!!", "Installing game, please wait... ", "Done"
	},
    // spanish
	{"Retorna", "Configurar", "Borra Favorito", "A�ade Favorito", "Carga juego", "A�ade a Favoritos", "Favoritos", "P�gina", "Hecho", "Descartar",
	" C�digos de Trucos encontrados !!! ","Usa C�digos", "Salta C�digos", "Formatear WBFS", "Seleccionado", "Ya tienes una partici�n WBFS", 
	"�Estas seguro que quieres formatear?", " Si ", " No ", "Formateando...", "Exito Formateando el Disco", "Error al Formatear el Disco",
//22	
	"Retornar", "A�adir Nuevo Juego", "A�adir Bitmap PNG", "Borrar Juego", "Formatear Disco", "Retorna al Menu de Wii", "","","","",
//32
	"�Est�s seguro de que quieres borrar �ste?", "Pulsa A para aceptar o B para cancelar",
// 34
"Inserta el DVD del juego ...", "ERROR! Abortado", "Pulsa B para Abortar","Abriendo el disco DVD...", "ERROR: No es un disco de Wii!!",
"ERROR: Juego ya instalado!!!", "Instalando el juego, espera... ", "Terminado"

	},
	};

 
																     
//---------------------------------------------------------------------------------
/* Reset and Power Off */
//---------------------------------------------------------------------------------

int exit_by_reset=0;

int return_reset=2;

void reset_call() {exit_by_reset=return_reset;}
void power_call() {exit_by_reset=3;}

//---------------------------------------------------------------------------------
/* from YAL loader */
//---------------------------------------------------------------------------------

GXRModeObj*		vmode;					// System Video Mode
unsigned int	_Video_Mode;				// System Video Mode (NTSC, PAL or MPAL)	
u32 *xfb; 

int forcevideo=0;

void Determine_VideoMode(char Region)
{
	u32 progressive;
	// Get vmode and Video_Mode for system settings first
	u32 tvmode = CONF_GetVideo();
	// Attention: This returns &TVNtsc480Prog for all progressive video modes
        vmode = VIDEO_GetPreferredMode(0);

	switch(forcevideo)
	{
	case 0:
				switch (tvmode) 
				{
					case CONF_VIDEO_PAL:
							if (CONF_GetEuRGB60() > 0) 
									_Video_Mode = PAL60;
							else 
									_Video_Mode = PAL;
							break;
					case CONF_VIDEO_MPAL:
							_Video_Mode = MPAL;
							break;

					case CONF_VIDEO_NTSC:
					default:
							_Video_Mode = NTSC;
				}

			// Overwrite vmode and Video_Mode when disc region video mode is selected and Wii region doesn't match disc region
				switch (Region) 
				{
				case PAL_Default:
				case PAL_France:
				case PAL_Germany:
				case Euro_X:
				case Euro_Y:
						if (CONF_GetVideo() != CONF_VIDEO_PAL)
						{
								_Video_Mode = PAL60;

								if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
										vmode = &TVNtsc480Prog; // This seems to be correct!
								else
										vmode = &TVEurgb60Hz480IntDf;
						}
						break;

				case NTSC_USA:
				case NTSC_Japan:
						if (CONF_GetVideo() != CONF_VIDEO_NTSC)
						{
								_Video_Mode = NTSC;
								if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
										vmode = &TVNtsc480Prog;
								else
										vmode = &TVNtsc480IntDf;
						}
				default:
						break;
				}
				break;

		 case 1:
				/* GAME LAUNCHED WITH 1 - FISHEARS*/
                _Video_Mode = 5;
                progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
                vmode     = (progressive) ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
                break;
         case 2:
                /* GAME LAUNCHED WITH 2 - FISHEARS*/
                _Video_Mode = 0;
                progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
                vmode     = (progressive) ? &TVNtsc480Prog : &TVNtsc480IntDf;
                break;
		}		

/* Set video mode register */
	*(vu32 *)0x800000CC = _Video_Mode;

	
}

void Set_VideoMode(void)
{
    // TODO: Some exception handling is needed here
 
    // The video mode (PAL/NTSC/MPAL) is determined by the value of 0x800000cc
    // The combination Video_Mode = NTSC and vmode = [PAL]576i, results in an error
    
    *Video_Mode = _Video_Mode;

    VIDEO_Configure(vmode);
	//VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}

#define CERTS_SIZE	0xA00

static const char certs_fs[] ALIGNED(32) = "/sys/cert.sys";

s32 GetCerts(signed_blob** Certs, u32* Length)
{
	static unsigned char		Cert[CERTS_SIZE] ALIGNED(32);
	memset(Cert, 0, CERTS_SIZE);
	s32				fd, ret;

	fd = IOS_Open(certs_fs, ISFS_OPEN_READ);
	if (fd < 0) return fd;

	ret = IOS_Read(fd, Cert, CERTS_SIZE);
	if (ret < 0)
	{
		if (fd >0) IOS_Close(fd);
		return ret;
	}

	*Certs = (signed_blob*)(Cert);
	*Length = CERTS_SIZE;

	if (fd > 0) IOS_Close(fd);

	return 0;
}



int cios = CIOS;

//---------------------------------------------------------------------------------
/* from Waninkoko usbloader */
//---------------------------------------------------------------------------------


/* Gamelist variables */
static struct discHdr *gameList = NULL;

static u32 gameCnt = 0;


s32 __Menu_EntryCmp(const void *a, const void *b)
{
	struct discHdr *hdr1 = (struct discHdr *)a;
	struct discHdr *hdr2 = (struct discHdr *)b;

	/* Compare strings */
	return strcmp(hdr1->title, hdr2->title);
}
//---------------------------------------------------------------------------------
/* Wiimote's routines */
//---------------------------------------------------------------------------------

unsigned temp_pad=0,new_pad=0,old_pad=0;

WPADData * wmote_datas=NULL;

// wiimote position (temp)
int px=-100,py=-100;

// wiimote position emulated for Guitar
int guitar_pos_x,guitar_pos_y;


unsigned wiimote_read()
{
int n;

int ret=-1;

unsigned type=0;

unsigned butt=0;

wmote_datas=NULL;


for(n=0;n<4;n++) // busca el primer wiimote encendido y usa ese
	{
	ret=WPAD_Probe(n, &type);

	if(ret>=0)
		{
		
		butt=WPAD_ButtonsHeld(n);
		
			wmote_datas=WPAD_Data(n);
		
		return butt;
		}
	}

return 0;
}


//---------------------------------------------------------------------------------
/* Sound effects */
//---------------------------------------------------------------------------------

void snd_fx_tick()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(2, VOICE_MONO_8BIT, 4096*6*2,0, &sound_effects[0][0], 2048/128, 64, 64, NULL);
#endif
}
void snd_fx_yes()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(1, VOICE_MONO_8BIT, 12000,0, &sound_effects[1][0], 2048/4, 127, 127, NULL);
#endif
}

void snd_fx_no()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(1, VOICE_MONO_8BIT, 4096,0, &sound_effects[0][0], 2048/16, 127, 127, NULL);
#endif
}

void snd_fx_scroll()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(2, VOICE_MONO_8BIT, 1024,0, &sound_effects[0][0], 64, 64, 64, NULL);
#endif
}



void snd_fx_fx(int percent)
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(2, VOICE_MONO_8BIT, 5000+percent*250,0, &sound_effects[0][0], 128, 64, 64, NULL);
#endif
}
//---------------------------------------------------------------------------------
/* GUI routines and datas*/
//---------------------------------------------------------------------------------

int scroll_text=-20;

GXTlutObj palette_icon;
GXTexObj text_icon[2];

GXTexObj text_button[4], default_game_texture, text_background,text_game_empty[4];
GXTexObj text_screen_fx;

u32 *screen_fx=NULL;

GXTexObj text_move_chan;

GXTexObj png_texture;

void *mem_move_chan= NULL;

struct _game_datas
{
	int ind;
	void * png_bmp;
	GXTexObj texture;
	u32 config;
} game_datas[16];


void DrawIcon(int px,int py, u32 frames2)
{
	DrawSurface(&text_icon[(frames2 & 4)!=0 && ((frames2 & 128)>110 || (frames2 & 32)!=0)],px-24,py-24, 48, 48, 0, 0xffffffff);
}

void draw_text(char *text)
{
int n,m,len;
len=strlen(text);


for(n=0;n<17;n++)
	{
    if(scroll_text<0) m=n;
	else
		m=(n+scroll_text) % (len+4);

	if(m<len)
		s_printf("%c",text[m]);
	else
		s_printf(" ");
	}
}

void draw_box_text(char *text)
{
int n,m,len,len2;

len=strlen(text);
len2=len/68;

for(n=0;n<68;n++)
	{
	if(len<=68 || scroll_text<0) m=n;
	else m=(n+scroll_text) % (len+4); //m=n+68*((scroll_text>>5)  % len2);

	if(m<len)
		s_printf("%c",text[m]);
	else
		s_printf(" ");
	}
}

// Draw_button variables

int step_button=0;

int x_temp=0;

int Draw_button(int x,int y,char *cad)
{
int len=strlen(cad);

	SetTexture(&text_button[(step_button>>4) & 3]);
	
	if(px>=x && px<=x+len*8+32 && py>=y && py<y+56) DrawRoundFillBox(x-8, y-8, len*8+32+16, 56+16, 0, 0xffcfcfcf);
		else DrawRoundFillBox(x, y, len*8+32, 56, 0, 0xffcfcfcf);
	SetTexture(NULL);
	PX=x+16; PY= y+12; color= 0xff000000;
	letter_size(8,32);

	s_printf("%s",cad);

	x_temp=x+len*8+32;

	if(px>=x && px<=x+len*8+32 && py>=y && py<y+56)
		{DrawRoundBox(x-8, y-8, len*8+32+16, 56+16, 0, 5, 0xfff08000);return 1;}
	
	DrawRoundBox(x, y, len*8+32, 56, 0, 4, 0xff606060);
	

return 0;
}

int Draw_button2(int x,int y,char *cad, int selected)
{
int len=strlen(cad);
unsigned color=0xffcfcfcf;

if(selected) color= 0xff3fcf3f;

if(selected<0) color=0x80cfcfcf;

	SetTexture(&text_button[(step_button>>4) & 3]);
	
	if(selected>=0)
		{
		if(px>=x && px<=x+len*8+32 && py>=y && py<y+48) DrawRoundFillBox(x-8, y-8, len*8+32+16, 48+16, 0, color);
			else DrawRoundFillBox(x, y, len*8+32, 48, 0, color);
		}

	SetTexture(NULL);
	PX=x+16; PY= y+8; color= 0xff000000;
	letter_size(8,32);

	s_printf("%s",cad);

	x_temp=x+len*8+32;

	if(selected>=0)
		if(px>=x && px<=x+len*8+32 && py>=y && py<y+48)
			{DrawRoundBox(x-8, y-8, len*8+32+16, 48+16, 0, 5, 0xfff08000);return 1;}
	
	DrawRoundBox(x, y, len*8+32, 48, 0, 4, 0xff606060);
	

return 0;
}


void * create_png_texture(GXTexObj *texture, void *png, int repeat)
{
PNGUPROP imgProp;
IMGCTX ctx;
char *texture_buff;
s32 ret;

	/* Select PNG data */
	ctx = PNGU_SelectImageFromBuffer(png);
	if (!ctx)
		{return NULL;}

	/* Get image properties */
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK)
		{return NULL;}

    texture_buff=memalign(32, imgProp.imgWidth * imgProp.imgHeight *4+2048);
	if(!texture_buff) {return NULL;}

	
	PNGU_DecodeTo4x4RGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, texture_buff, 255);
	PNGU_ReleaseImageContext(ctx);

	DCFlushRange( texture_buff, imgProp.imgWidth * imgProp.imgHeight *4);

	if(repeat) GX_InitTexObj(texture,texture_buff, imgProp.imgWidth, imgProp.imgHeight, GX_TF_RGBA8, GX_REPEAT ,  GX_REPEAT , GX_FALSE);
	else GX_InitTexObj(texture,texture_buff, imgProp.imgWidth, imgProp.imgHeight, GX_TF_RGBA8, GX_CLAMP ,  GX_CLAMP , GX_FALSE);

	GX_InitTexObjLOD(texture, // objeto de textura
						 GX_LINEAR, // filtro Linear para cerca
						 GX_LINEAR, // filtro Linear para lejos
						 0, 0, 0, 0, 0, GX_ANISO_1);

return texture_buff;
}


void create_game_png_texture(int n)
{
	

	PNGUPROP imgProp;
	IMGCTX ctx;
	char *texture_buff;

	s32 ret;


	if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
		{game_datas[n].png_bmp=NULL;return;}

    game_datas[n].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);

	if(!(temp_data[9]=='P' && temp_data[10]=='N' && temp_data[11]=='G'))
		{game_datas[n].png_bmp=NULL;return;}

	/* Select PNG data */
	ctx = PNGU_SelectImageFromBuffer(temp_data+8);
	if (!ctx)
		{game_datas[n].png_bmp=NULL;return;}

	/* Get image properties */
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK)
		{game_datas[n].png_bmp=NULL;return;}

    texture_buff=memalign(32, imgProp.imgWidth * imgProp.imgHeight *4+2048);
	if(!texture_buff) {game_datas[n].png_bmp=NULL;return;}

	
	PNGU_DecodeTo4x4RGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, texture_buff, 255);
	PNGU_ReleaseImageContext(ctx);

	game_datas[n].png_bmp=texture_buff;
	
    DCFlushRange( texture_buff, imgProp.imgWidth * imgProp.imgHeight *4);

	GX_InitTexObj(&game_datas[n].texture,texture_buff, imgProp.imgWidth, imgProp.imgHeight, GX_TF_RGBA8, GX_CLAMP ,  GX_CLAMP , GX_FALSE);
	GX_InitTexObjLOD(&game_datas[n].texture, // objeto de textura
						 GX_LINEAR, // filtro Linear para cerca
						 GX_LINEAR, // filtro Linear para lejos
						 0, 0, 0, 0, 0, GX_ANISO_1);
		
}


//---------------------------------------------------------------------------------
/* Configuration datas (Favorites) */
//---------------------------------------------------------------------------------

struct _config_file
{
	u32 magic;
	u8 id[16][8];
	u32 pad[31];
}
config_file ATTRIBUTE_ALIGN(32);

int sd_ok=0;

void load_cfg()
{
FILE *fp=0;
int n=0;

	config_file.magic=0;

    // Load CFG from HDD (or create __CFG_ disc)
	if(WBFS_LoadCfg(&config_file,sizeof (config_file)))
	{
	if(config_file.magic!=0xcacafea1)
		{
		memset(&config_file,0, sizeof (config_file));
		}
	else
		{
		int n,m;

		// delete not found entries in disc
		for(m=0;m<16;m++)
			{
			for(n=0;n<gameCnt;n++)
				{
			    struct discHdr *header = &gameList[n];
				  if(strncmp((char *) header->id, (char *) &config_file.id[m][0],6)==0) break;
				}
			if(n==gameCnt) memset(&config_file.id[m][0],0,6);
			
			}
		}
	return;
	}

	if(sd_ok)
	{

	fp=fopen("sd:/apps/uloader/uloader.cfg","rb"); // lee el fichero de texto
	if(!fp)
		fp=fopen("sd:/uloader/uloader.cfg","rb"); // lee el fichero de texto
	if(!fp)
		fp=fopen("sd:/uloader.cfg","rb"); // lee el fichero de texto

	if(fp!=0)
		{
		n=fread(&config_file,1, sizeof (config_file) ,fp);
		fclose(fp);
		}
	}

	if(n!=sizeof (config_file) || !fp || config_file.magic!=0xcacafea1)
		{
		memset(&config_file,0, sizeof (config_file));
		}
}

void save_cfg()
{
FILE *fp;

config_file.magic=0xcacafea1;

if(WBFS_SaveCfg(&config_file,sizeof (config_file)))
	{
	return;
	}

if(!sd_ok) return;

	
	fp=fopen("sd:/apps/uloader/uloader.cfg","wb"); // escribe el fichero de texto
	if(!fp)
		fp=fopen("sd:/uloader/uloader.cfg","wb"); // escribe el fichero de texto
	if(!fp)
		fp=fopen("sd:/uloader.cfg","wb"); // escribe el fichero de texto

	if(fp!=0)
		{
		fwrite(&config_file,1, sizeof (config_file) ,fp);
		fclose(fp);
		}

}

#define MAX_LIST_CHEATS 25

int txt_cheats=0;

int num_list_cheats=0;
int actual_cheat=0;

void create_cheats();

struct _data_cheats
{
void *title;
void *description;

int apply;
u8 *values;
int len_values;

} data_cheats[MAX_LIST_CHEATS];

extern int abort_signal;

void display_spinner(int mode, int percent, char *str)
{
//Screen_flip();

	autocenter=1;
	SetTexture(&text_background);
	DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, 0xffa0a0a0);

	SetTexture(NULL);
	
	DrawFillSlice(SCR_WIDTH/2, SCR_HEIGHT/2, 210, 210, 10, 0, 360, 0x8f60a0a0);
	SetTexture(MOSAIC_PATTERN);
	DrawFillSlice(SCR_WIDTH/2, SCR_HEIGHT/2, 210, 210, 10, 0, 360*percent/100, 0xffa06000);
	SetTexture(NULL);
	DrawSlice(SCR_WIDTH/2, SCR_HEIGHT/2, 210, 210, 10, 4, 0, 360,  0xcf000000);
    

	PX=0; PY=SCR_HEIGHT/2-16; color= 0xff000000; 
	letter_size(16,32);
	SelectFontTexture(1);
	if(mode)
		color=0xffffffff;
	else
		color=0xff000000;

	s_printf("%s", str);
	color=0xff000000;
	autocenter=0;
	
	Screen_flip();

// abort
if(exit_by_reset) {exit_by_reset=0;abort_signal=1;}
SYS_SetResetCallback(reset_call); 

if(mode)
	sleep(4);


 
}
void my_perror(char * err)
{
	Screen_flip();
	autocenter=1;
	SetTexture(&text_background);
	DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, 0xffa0a0a0);

	SetTexture(NULL);
	DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-32, 540, 64, 999, 0xa00000ff);
	DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-32, 540, 64, 999, 4, 0xa0000000);

	PX=0; PY=SCR_HEIGHT/2-16; color= 0xff000000; 
	letter_size(8,32);
	SelectFontTexture(1);
	s_printf("Error: %s",err);
	autocenter=0;
	Screen_flip();
	sleep(4);
}

struct _partition_type
{
	int type;
	u32 lba, len;
}
partition_type[32];

int num_partitions=0;

u32 sector_size;

#define read_le32_unaligned(x) ((x)[0]|((x)[1]<<8)|((x)[2]<<16)|((x)[3]<<24))

int get_partitions()
{
u8 part_table[16*4];
int ret;
u8 *ptr;
int i;
int n_sectors;

	num_partitions=0;
	memset(partition_type, 0, sizeof(partition_type));


	n_sectors = USBStorage2_GetCapacity(&sector_size);
	if(n_sectors<0) return -1;

	n_sectors+=1; 
    
	ret = USBStorage2_ReadSectors(0, 1, temp_data);
    if(ret<0) return -2;
    
	///find wbfs partition
    memcpy(part_table,temp_data+0x1be,16*4);
    ptr = part_table;

	for(i=0;i<4;i++,ptr+=16)
        { 
		u32 part_lba = read_le32_unaligned(ptr+0x8);

		partition_type[num_partitions].len=read_le32_unaligned(ptr+0xC);
        partition_type[num_partitions].type=(0<<8) | ptr[4];
		partition_type[num_partitions].lba=part_lba;

		if(temp_data[0]=='W' && temp_data[1]=='B' && temp_data[2]=='F' && temp_data[3]=='S' && i==0)
			{partition_type[num_partitions].len=n_sectors;partition_type[num_partitions].type=(2<<8);num_partitions++; break;}


		if(ptr[4]==0) 
			{
			continue;
			}

		if(ptr[4]==0xf)
			{
			u32 part_lba2=part_lba;
			u32 next_lba2=0;
			int n;
			
			for(n=0;n<8;n++) // max 8 logic partitions (i think it is sufficient!)
				{
					ret = USBStorage2_ReadSectors(part_lba+next_lba2, 1, temp_data);
					if(ret<0)
						return -2; 

					part_lba2=part_lba+next_lba2+read_le32_unaligned(temp_data+0x1C6);
					next_lba2=read_le32_unaligned(temp_data+0x1D6);

					partition_type[num_partitions].len=read_le32_unaligned(temp_data+0x1CA);
					partition_type[num_partitions].lba=part_lba2;
					partition_type[num_partitions].type=(1<<8) | temp_data[0x1C2];

					ret = USBStorage2_ReadSectors(part_lba2, 1, temp_data);
					if(ret<0)
						return -2;

					
					if(temp_data[0]=='W' && temp_data[1]=='B' && temp_data[2]=='F' && temp_data[3]=='S')
						partition_type[num_partitions].type=(3<<8) | temp_data[0x1C2];
				
					num_partitions++;
					if(next_lba2==0) break;
				}
			}  
          else   
				{
					ret = USBStorage2_ReadSectors(part_lba, 1, temp_data);
			
					if(ret<0)
						return -2;

					if(temp_data[0]=='W' && temp_data[1]=='B' && temp_data[2]=='F' && temp_data[3]=='S')
						partition_type[num_partitions].type|=(2<<8);

					num_partitions++;
				}
        }
return 0;
}

int menu_format()
{
int frames2=0;
int n,m;
int ret;
char type[4][9]={"Primary ", "Extended", "WBFS Pri", "WBFS Ext"};

int current_partition=0;

int level=0;
int it_have_wbfs=0;

int last_select=-1;

re_init:

autocenter=0;
ret=get_partitions();
current_partition=0;
level=0;

if(ret==-1 /*|| num_partitions<=0*/) return -1;



while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	WPAD_ScanPads(); // esto lee todos los wiimotes

	SetTexture(&text_background);
   
	ConfigureForTexture(10);
	GX_Begin(GX_QUADS,  GX_VTXFMT0, 4);
	AddTextureVertex(0, 0, 999, 0xfff0f0f0, 0, (frames2 & 1023));
	AddTextureVertex(SCR_WIDTH, 0, 999, 0xffa0a0a0, 1023, (frames2 & 1023)); 
	AddTextureVertex(SCR_WIDTH, SCR_HEIGHT, 999, 0xfff0f0f0, 1023, 1024+(frames2 & 1023)); 
	AddTextureVertex(0, SCR_HEIGHT, 999, 0xffa0a0a0, 0, 1024+(frames2 & 1023)); 
	GX_End();
		

	SetTexture(NULL);
    DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

	SelectFontTexture(1); // selecciona la fuente de letra extra
    
	
	PX= 0; PY=ylev-32; color= 0xff000000; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=0;//0xb0f0f0f0;
	s_printf("%s", &letrero[idioma][13][0]);
	bkcolor=0;
	autocenter=0;
	

	color= 0xff000000; 
	letter_size(8,24);
	SetTexture(NULL);

	it_have_wbfs=0;

	for(n=0;n<num_partitions;n++)
		{
		if(type[partition_type[n].type>>8][0]>1) {it_have_wbfs=1;break;}
		}

	if(level>=100)
		{
		bkcolor=0;
		
		
		if(level>=1000)
			{
			PX= 0; PY=ylev+(352)/2-16;
			letter_size(16,32);
			autocenter=1;
			if(level>=2048+128)
				{
				DrawRoundFillBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 0xff00ff00);
				DrawRoundBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 4, 0xff000000);

				s_printf("%s", &letrero[idioma][20][0]);
				}
			else
			if(level>=2048)
				{
				DrawRoundFillBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 0xff0000ff);
				DrawRoundBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 4, 0xff000000);

				s_printf("%s", &letrero[idioma][21][0]);
				}
			else
				s_printf("%s", &letrero[idioma][19][0]);

			autocenter=0;
			}
		else
			{
			PX= 0; PY=ylev+32;
			letter_size(16,32);
			autocenter=1;
			s_printf("%s", &letrero[idioma][14][0]);
			letter_size(8,32);
			PX= ((SCR_WIDTH-54*8)>>1)-28; PY=ylev+80+32;
			m=level-100;autocenter=0;

			DrawRoundFillBox(PX-32, PY-12, 60*8+64, 56, 0, 0xcf0000ff);
							
			DrawRoundBox(PX-32, PY-12, 60*8+64, 56, 0, 4, 0xff000000);
			
			color= 0xff000000; 
			s_printf("Partition %2.2i -> %2.2xh (%s) LBA: %.10u LEN: %.2fGB", m+1, partition_type[m].type & 255, &type[partition_type[m].type>>8][0],
					partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
			
			autocenter=1;
			PX= 0; PY=ylev+180;
			letter_size(12,32);
			s_printf("%s", &letrero[idioma][16][0]);
		
			PX= 0; PY=ylev+180+48;
			if(it_have_wbfs)
				s_printf("%s", &letrero[idioma][15][0]);
			autocenter=0;

			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=60;
			
			if(Draw_button(600-32-strlen(&letrero[idioma][18][0])*8, ylev+108*4-64, &letrero[idioma][18][0])) select_game_bar=61;
			}
		}

    if(level==0)
		{
		for(n=0;n<6;n++)
			{
			PX=((SCR_WIDTH-54*8)>>1)-28; PY=ylev+32+n*50;
			m=(current_partition+n);
			if(m>=num_partitions)
				{
				DrawRoundFillBox(PX-32, PY-8, 60*8+64, 40, 0, 0xcf808080);
				DrawRoundBox(PX-32, PY-8, 60*8+64, 40, 0, 4, 0xff606060);
				if(num_partitions<1 && n==0)
					{
					s_printf("No Partitions in Disc Detected");
					}
				}
			else
				{
				if(px>=PX-32 && px<=PX+60*8+64 && py>=PY-8 && py<PY+32) 
					{
					if((partition_type[m].type>>8)>1) DrawRoundFillBox(PX-40, PY-12, 60*8+80, 48, 0, 0xff00cf00);
					else DrawRoundFillBox(PX-40, PY-12, 60*8+80, 48, 0, 0xffcfcfcf);
					
					DrawRoundBox(PX-40, PY-12, 60*8+80, 48, 0, 5, 0xfff08000);select_game_bar=100+m;
					}
				else 
					{
					if((partition_type[m].type>>8)>1)  DrawRoundFillBox(PX-32, PY-8, 60*8+64, 40, 0, 0xff00cf00);
					else DrawRoundFillBox(PX-32, PY-8, 60*8+64, 40, 0, 0xffcfcfcf);
					
					DrawRoundBox(PX-32, PY-8, 60*8+64, 40, 0, 4, 0xff606060);
					}
				
				s_printf("Partition %2.2i -> %2.2xh (%s) LBA: %.10u LEN: %.2fGB", m+1, partition_type[m].type & 255, &type[partition_type[m].type>>8][0],
					partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
				}
			}
		
	
			SetTexture(NULL);
		
			if(current_partition>=6)
				{
				if(px>=0 && px<=80 && py>=ylev+220-40 && py<=ylev+220+40)
					{
					DrawFillEllipse(40, ylev+220, 50, 50, 0, 0xc0f0f0f0);
					letter_size(32,64);
					PX= 40-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
					s_printf("-");
					DrawEllipse(40, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
					select_game_bar=50;
					}
				else
				if(frames2 & 32)
					{
					DrawFillEllipse(40, ylev+220, 40, 40, 0, 0xc0f0f0f0);
					letter_size(32,48);
					PX= 40-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
					s_printf("-");
					DrawEllipse(40, ylev+220, 40, 40, 0, 6, 0xc0000000);
					}
				}

				if((current_partition+6)<num_partitions)
				{
				if(px>=SCR_WIDTH-82 && px<=SCR_WIDTH-2 && py>=ylev+220-40 && py<=ylev+220+40)
					{
					DrawFillEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 0xc0f0f0f0);
					letter_size(32,64);
					PX= SCR_WIDTH-42-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
					s_printf("+");
					DrawEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
					select_game_bar=51;
					}
				else
				if(frames2 & 32)
					{
					DrawFillEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 0xc0f0f0f0);
					letter_size(32,48);
					PX= SCR_WIDTH-42-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
					s_printf("+");
					DrawEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 6, 0xc0000000);
					}
				}

			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;
			
			if(Draw_button(600-32-strlen(&letrero[idioma][0][0])*8, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;
		} // level==0

	frames2++;step_button++;

	if(select_game_bar>=0)
		{
		if(select_game_bar!=last_select)
			{
			snd_fx_tick();
			last_select=select_game_bar;
			}
		}
	else last_select=-1;
	

	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(level<1000)
		{

		if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(px<16) px=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}
						
						px=guitar_pos_x; py=guitar_pos_y;

						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						
						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) new_pad|=WPAD_BUTTON_A;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) old_pad|=WPAD_BUTTON_A;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) new_pad|=WPAD_BUTTON_1;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) old_pad|=WPAD_BUTTON_1;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) new_pad|=WPAD_BUTTON_MINUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) old_pad|=WPAD_BUTTON_MINUS;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) new_pad|=WPAD_BUTTON_PLUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) old_pad|=WPAD_BUTTON_PLUS;

						}
					 else
					   {px=-100;py=-100;}

				if(new_pad & WPAD_BUTTON_B)
					{
					snd_fx_no();
					if(level==0) break;
					else level=0;
					}

				if((new_pad &  WPAD_BUTTON_MINUS) && level==0)
					{
					current_partition-=6;if(current_partition<0) current_partition=0;
					}
				
				if((new_pad &  WPAD_BUTTON_PLUS) && level==0)
					{
					current_partition+=6;if(current_partition>(num_partitions+6)) current_partition-=6;
					}

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==1) {snd_fx_yes();break;}
					if(select_game_bar==50) {snd_fx_yes();current_partition-=6;if(current_partition<0) current_partition=0;} //page -6
					if(select_game_bar==51) {snd_fx_yes();current_partition+=6;if(current_partition>(num_partitions+6)) current_partition-=6;} // page +6

					if(select_game_bar>=100 && select_game_bar<150) // select partition to format
						{
						level=select_game_bar;snd_fx_yes();
						}
					
					if(select_game_bar==60) {level+=900;snd_fx_yes();} // Yes
					if(select_game_bar==61) {snd_fx_no();level=0;} // No

                   
					}
				}

		}
	

	Screen_flip();
	
	if(level>=1000)
		{
		if(level<2000)
			{
			if(WBFS_Format(partition_type[level-1000].lba, partition_type[level-1000].len)<0) level=2048; // error
			else level=2048+128; // ok
			}

        level++;
		if((level & 127)>120) 
			{	 
			if(level>=2048+128) break;
			level=0;goto re_init;
			}
		}


	if(exit_by_reset) break;
	}

return 0;
}

void cabecera(int frames2, char *cab)
{
	int ylev=(SCR_HEIGHT-440);


	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	SetTexture(&text_background);
   
	ConfigureForTexture(10);
	GX_Begin(GX_QUADS,  GX_VTXFMT0, 4);
	AddTextureVertex(0, 0, 999, 0xfff0f0f0, 0, (frames2 & 1023));
	AddTextureVertex(SCR_WIDTH, 0, 999, 0xffa0a0a0, 1023, (frames2 & 1023)); 
	AddTextureVertex(SCR_WIDTH, SCR_HEIGHT, 999, 0xfff0f0f0, 1023, 1024+(frames2 & 1023)); 
	AddTextureVertex(0, SCR_HEIGHT, 999, 0xffa0a0a0, 0, 1024+(frames2 & 1023)); 
	GX_End();
		

	SetTexture(NULL);
    DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

	PX= 0; PY=ylev-32; color= 0xff000000; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=num_partitions=0;//0xb0f0f0f0;
	s_printf("%s", cab);
	bkcolor=0;
	

}


void add_game()
{
int frames2=0;
int ret;
static struct discHdr header ATTRIBUTE_ALIGN(32);
char str_id[7];	
//ASND_Pause(1);

    
	WDVD_SetUSBMode(NULL);

	WBFS_Init();

	{
	int ylev=(SCR_HEIGHT-440);


	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;
	
	while(1)
		{

		cabecera(frames2, &letrero[idioma][23][0]);
	
		PX=0;PY=ylev+352/2-16;
		s_printf("%s",&letrero[idioma][34][0]);
		PX=0;PY+=64;
		s_printf("%s",&letrero[idioma][36][0]);
	
		WPAD_ScanPads(); // esto lee todos los wiimotes
		temp_pad= wiimote_read(); 
		new_pad=temp_pad & (~old_pad);old_pad=temp_pad;
		
		Screen_flip();

	    if(new_pad & (WPAD_GUITAR_HERO_3_BUTTON_RED | WPAD_BUTTON_B))
			{
			cabecera(frames2, &letrero[idioma][23][0]);
			PX=0;PY=ylev+352/2-16;

			s_printf("%s",&letrero[idioma][35][0]);
			snd_fx_no();
			Screen_flip();
			autocenter=0;
			sleep(4);
			goto out;
			}
	     /* Wait for disc */
		ret = Disc_Wait();
		if(ret==0) break;
		if (ret < 0) 
			{
			cabecera(frames2, &letrero[idioma][23][0]);
			PX=0;PY=ylev+352/2-16;
			s_printf("ERROR! (ret = %d)", ret);
			snd_fx_no();
			Screen_flip();
			autocenter=0;
			sleep(4);
			goto out;
			}
		}

	cabecera(frames2, &letrero[idioma][23][0]);
	PX=0;PY=ylev+352/2-16;

	s_printf("%s",&letrero[idioma][37][0]);
	Screen_flip();	

		/* Open disc */
	ret = Disc_Open();
	if (ret < 0) {
		cabecera(frames2, &letrero[idioma][23][0]);
		PX=0;PY=ylev+352/2-16;
		s_printf("ERROR! (ret = %d)", ret);
		Screen_flip();
		snd_fx_no();
		autocenter=0;
		sleep(4);
		goto out;
	}
  
	/* Check disc */
	ret = Disc_IsWii();
	if (ret < 0) {

		cabecera(frames2, &letrero[idioma][23][0]);
		PX=0;PY=ylev+352/2-16;
		s_printf("%s",&letrero[idioma][38][0]);
		Screen_flip();
		snd_fx_no();
		autocenter=0;
		sleep(4);
		goto out;
	
	}


	/* Read header */
	Disc_ReadHeader(&header);

	
	memcpy(str_id,header.id,6); str_id[6]=0;
	
	/* Check if game is already installed */
	ret = WBFS_CheckGame((u8*) str_id);
	if (ret) {
		cabecera(frames2, &letrero[idioma][23][0]);
		PX=0;PY=ylev+352/2-16;
		
		s_printf("%s",&letrero[idioma][39][0]);
		Screen_flip();
		snd_fx_no();
		autocenter=0;
		sleep(4);
		goto out;

	}

/////////////////////


    while(1)
		{
	
		WPAD_ScanPads(); // esto lee todos los wiimotes
		temp_pad= wiimote_read(); 
		new_pad=temp_pad & (~old_pad);old_pad=temp_pad;
		
		cabecera(frames2, &letrero[idioma][23][0]);

		if(strlen(header.title)<=37) letter_size(16,32);
		else if(strlen(header.title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX=0;PY=ylev+352/2-32; color= 0xff000000; 
				
		bkcolor=0;
		
		s_printf("%s",header.title);
		
		letter_size(12,32);

		PX=0;PY=ylev+352/2+32; 

		s_printf(&letrero[idioma][33][0]);

		Screen_flip();

	    if(new_pad & (WPAD_GUITAR_HERO_3_BUTTON_RED | WPAD_BUTTON_B))
			{
			cabecera(frames2, &letrero[idioma][23][0]);
			PX=0;PY=ylev+352/2-16;
			s_printf("%s",&letrero[idioma][35][0]);
			snd_fx_no();
			Screen_flip();
			autocenter=0;
			sleep(4);
			goto out;
			}
		if(new_pad & (WPAD_GUITAR_HERO_3_BUTTON_GREEN | WPAD_BUTTON_A)) {snd_fx_yes();break;}
		}
    

////////////////////
	cabecera(frames2, &letrero[idioma][23][0]);
	PX=0;PY=ylev+352/2-16;
	
	s_printf("%s",&letrero[idioma][40][0]);
	snd_fx_yes();
	Screen_flip();	
    sleep(1);

	USBStorage2_Watchdog(0); // to increase the speed

	/* Install game */
	ret = WBFS_AddGame(0);

	USBStorage2_Watchdog(1); // to avoid hdd sleep

	if(exit_by_reset) {exit_by_reset=0;}

	if (ret < 0) {
		if(ret==-666) goto out;
		cabecera(frames2, &letrero[idioma][23][0]);
		PX=0;PY=ylev+352/2-16;
		s_printf("Installation ERROR! (ret = %d)", ret);
		snd_fx_no();
		Screen_flip();
		autocenter=0;
		sleep(4);
		goto out;
	}

	cabecera(frames2, &letrero[idioma][23][0]);
	PX=0;PY=ylev+352/2-16;

	
	s_printf("%s",&letrero[idioma][41][0]);
	snd_fx_yes();
	Screen_flip();
	sleep(4);
	}
autocenter=0;

out:
	autocenter=0;
	WBFS_Close();
}


int delete_game(char *name)
{
int frames2=0;

while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	SetTexture(&text_background);


	ConfigureForTexture(10);
	GX_Begin(GX_QUADS,  GX_VTXFMT0, 4);
	AddTextureVertex(0, 0, 999, 0xfff0f0f0, 0, (frames2 & 1023));
	AddTextureVertex(SCR_WIDTH, 0, 999, 0xffa0a0a0, 1023, (frames2 & 1023)); 
	AddTextureVertex(SCR_WIDTH, SCR_HEIGHT, 999, 0xfff0f0f0, 1023, 1024+(frames2 & 1023)); 
	AddTextureVertex(0, SCR_HEIGHT, 999, 0xffa0a0a0, 0, 1024+(frames2 & 1023)); 
	GX_End();

	SetTexture(NULL);
    DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

	PX= 0; PY=ylev-32; color= 0xff000000; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=num_partitions=0;//0xb0f0f0f0;
	s_printf("%s", &letrero[idioma][25][0]);
	bkcolor=0;
	
	PX=0;PY=ylev+352/2-32;
	autocenter=1;letter_size(12,32);
	s_printf("%s", &letrero[idioma][32][0]);

	if(strlen(name)<=37) letter_size(16,32);
	else if(strlen(name)<=45) letter_size(12,32);
	else letter_size(8,32);		

	PX=0;PY=ylev+352/2+32; color= 0xff000000; 
			
	bkcolor=0;
	
	s_printf("%s",name);
	autocenter=0;
	letter_size(16,32);

	
	if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=60;
			
	if(Draw_button(600-32-strlen(&letrero[idioma][18][0])*8, ylev+108*4-64, &letrero[idioma][18][0])) select_game_bar=61;
		

	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(px<16) px=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}
						
						px=guitar_pos_x; py=guitar_pos_y;

						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						
						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) new_pad|=WPAD_BUTTON_A;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) old_pad|=WPAD_BUTTON_A;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) new_pad|=WPAD_BUTTON_1;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) old_pad|=WPAD_BUTTON_1;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) new_pad|=WPAD_BUTTON_MINUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) old_pad|=WPAD_BUTTON_MINUS;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) new_pad|=WPAD_BUTTON_PLUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) old_pad|=WPAD_BUTTON_PLUS;

						}
					 else
					   {px=-100;py=-100;}

				if(new_pad & WPAD_BUTTON_B)
					{
					Screen_flip();snd_fx_no();break;
					}

				

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==60) {Screen_flip();snd_fx_yes();return 1;} // Yes
					if(select_game_bar==61) {Screen_flip();snd_fx_no();break;} // No
                   
					}
				}

	

	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	}

// No!!!

return 0;
}

void select_file(int flag, struct discHdr *header);

int home_menu(struct discHdr *header)
{
int frames2=0;
int n;
int old_select=-1;
char *punt;
char buff[64];

	while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	SetTexture(&text_background);


	ConfigureForTexture(10);
	GX_Begin(GX_QUADS,  GX_VTXFMT0, 4);
	AddTextureVertex(0, 0, 999, 0xfff0f0f0, 0, (frames2 & 1023));
	AddTextureVertex(SCR_WIDTH, 0, 999, 0xffa0a0a0, 1023, (frames2 & 1023)); 
	AddTextureVertex(SCR_WIDTH, SCR_HEIGHT, 999, 0xfff0f0f0, 1023, 1024+(frames2 & 1023)); 
	AddTextureVertex(0, SCR_HEIGHT, 999, 0xffa0a0a0, 0, 1024+(frames2 & 1023)); 
	GX_End();

	if(header)
		{
		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=ylev; color= 0xff000000; 
				
		bkcolor=0;
		
		autocenter=1;
		s_printf("%s",header->title);
		autocenter=0;
		}

		letter_size(16,32);
		

    for(n=0;n<6;n++)
		{
		memset(buff,32,56);buff[56]=0;
		punt=&letrero[idioma][22+n][0];
		memcpy(buff+(56-strlen(punt))/2, punt, strlen(punt));
	
		if(Draw_button2(30+48, ylev+56+64*n, buff,(!header && (n==2 || n==3)) ? -1 : 0)) select_game_bar=n+1;
		}
	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;

	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(px<16) px=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}
						
						px=guitar_pos_x; py=guitar_pos_y;

						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						
						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) new_pad|=WPAD_BUTTON_A;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) old_pad|=WPAD_BUTTON_A;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) new_pad|=WPAD_BUTTON_1;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) old_pad|=WPAD_BUTTON_1;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) new_pad|=WPAD_BUTTON_MINUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) old_pad|=WPAD_BUTTON_MINUS;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) new_pad|=WPAD_BUTTON_PLUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) old_pad|=WPAD_BUTTON_PLUS;

						}
					 else
					   {px=-100;py=-100;}

				
				if(new_pad & (WPAD_BUTTON_B | WPAD_BUTTON_HOME))
					{
					snd_fx_no();Screen_flip();
					return 0;
					}


				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==1) {snd_fx_yes();Screen_flip(); return 0;}
					if(select_game_bar==2) {snd_fx_yes();Screen_flip(); add_game(); return 2;}
				
					if(header)
						{
						if(select_game_bar==3) {snd_fx_yes();Screen_flip(); select_file(0,header); return 3;}
						if(select_game_bar==4) {snd_fx_yes();Screen_flip(); if(delete_game(header->title)) WBFS_RemoveGame(header->id);WBFS_Close();return 2;}
						}
					if(select_game_bar==5) {snd_fx_yes();WBFS_Close();Screen_flip(); if(menu_format()==0) return 2; else return 1;}
					
					if(select_game_bar==6) {snd_fx_yes();Screen_flip(); return 1;}
					}
			}
	
	frames2++;step_button++;
	Screen_flip();
	if(exit_by_reset)  
		{	
		break;
		}
	}

return 0;
}

void set_tex_pix_fx(int x, int y, unsigned color)
{
if(x<0 || x>127 || y<0 || y>127) return;

	screen_fx[x+(y<<7)]=color;
}
void set_text_screen_fx()
{
int n,m;
static int pos=256;
unsigned color1=0x80ffffff;

if(pos<-128) pos=1024;
memset(screen_fx,0,128*128*4);

for(n=0;n<128;n++)
	{

	color1=0x40ffffc0;
    for(m=0;m<4;m++)
		set_tex_pix_fx(pos-(n>>1)+m,n,color1);
	color1=((0x50-m)<<24) | 0xffffd0;
	for(m=8;m<20;m++)
		set_tex_pix_fx(pos-(n>>1)+m,n,color1);
	color1=0x40ffffc0;
	for(m=24;m<28;m++)
		set_tex_pix_fx(pos-(n>>1)+m,n,color1);
	
	}

CreateTexture(&text_screen_fx, TILE_SRGBA8, screen_fx, 128, 128, 0);
pos-=6;
}
//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
        u8 discid[7];

		int ret,ret2;
		u32 cnt, len;
		struct discHdr *buffer = NULL;
		int frames=0,frames2=0;
		int load_png=1;
		int scroll_mode=0;
		int game_mode=0;
		int edit_cfg=0;
		int cheat_mode=0;
		int select_game_bar=0;

		int is_favorite=1;
		int insert_favorite=0;
		int actual_game=0;
		int last_game=-1;

		int test_favorite=0;
		int n;
		int force_ios249=0;
		static int old_temp_sel=-1;
		f32 f_size=0.0f;
		int last_select=-1;

		int force_ingame_ios=0;

        return_reset=1;

		if(argc<1) return_reset=2;

        SYS_SetResetCallback(reset_call); // esto es para que puedas salir al pulsar boton de RESET
		SYS_SetPowerCallback(power_call); // esto para apagar con power
		
        discid[6]=0;
	
		ret2=-1;
	
		ret=IOS_ReloadIOS(cios);
		//ret=-1;	

		if(ret!=0)
			{
			force_ios249=1;
			cios=249;
			ret=IOS_ReloadIOS(cios);
			}

		sleep(1);
		load_ehc_module();
        
		

        // sound pattern generator
		for(n=0;n<2;n++)
		{
		int m,l;
		switch(n)
			{
			case 0:
				l=64;
				for(m=0;m<2048;m++)
				 {
			     sound_effects[0][m]=((m) & l) ? 127 : -128;
				 if((m & 255)==0) l>>=1; if(l<16) l=16;
				 }
			break;

			case 1:
				l=127;
				for(m=0;m<2048;m++)
				 {
			     sound_effects[1][m]=((m) & 8) ? l : -l;
				 if((m & 7)==0) l--; if(l<0) l=0;
				 }
			break;

			}
		}
		

		for(n=0;n<25;n++)
		{
			ret2 = USBStorage2_Init(); 
			if(!ret2) break;
			usleep(200*1000);
		}
	
		

		if(CONF_Init()==0)
		{
			switch(CONF_GetLanguage())
			{
			case CONF_LANG_SPANISH:
						idioma=1;break;
			default:
						idioma=0;break;
			}
			
		}
		
		//fatInit(8, true);
		__io_wiisd.startup();
		sd_ok = fatMountSimple("sd", &__io_wiisd);
		
		InitScreen();  // Inicializaci�n del V�deo
		
		create_png_texture(&text_background, background, 1);
		
		bkcolor=0;

		if(ret2>=0) 
			{
		    ret2=WBFS_Init();
			if(ret2>=0) ret2=Disc_Init();
			}
		
		
   
		for(n=0;n<3;n++)
			{
			autocenter=1;
			SetTexture(&text_background);
			DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, 0xffa0a0a0);
			PX=0; PY= 16; color= 0xff000000; 
			letter_size(32,64);
			SelectFontTexture(1);
			s_printf("%s","uLoader");
			SetTexture(NULL);
			DrawRoundBox((SCR_WIDTH-260)/2, 16, 260, 64, 0, 4, 0xff000000);

			SelectFontTexture(0);
			PY+=80;
			letter_size(16,32);
			s_printf("%s","\251 2009, Hermes (www.elotrolado.net)");
			PY+=40;
			
			s_printf("%s","Based in YAL \251 2009, Kwiirk");
			PY+=40;
			s_printf("%s","and USBLoader \251 2009, Waninkoko");
			PY+=34;
			letter_size(8,32);
			SelectFontTexture(1);
			s_printf("%s","Ocarina and some game patch added from FISHEARS usbloader version");
			
			autocenter=0;
			PX=20; PY= 32; color= 0xff000000; 
			letter_size(8,16);
			SelectFontTexture(1);
			s_printf("v1.5");
			PX=SCR_WIDTH-20-32;
			s_printf("v1.5");
			autocenter=1;
			if(n!=2) Screen_flip();
			}

		sleep(3);
	   
	   

		screen_fx=memalign(32, 128*128*4);

		CreatePalette(&palette_icon, TLUT_SRGB5A1, 0, icon_palette, icon_palette_colors); // crea paleta 0
		CreateTexture(&text_icon[0], TILE_CI8, icon_sprite_1, icon_sprite_1_sx, icon_sprite_1_sy, 0);
		CreateTexture(&text_icon[1], TILE_CI8, icon_sprite_2, icon_sprite_2_sx, icon_sprite_2_sy, 0);

		create_png_texture(&text_button[0], button1, 0);
		create_png_texture(&text_button[1], button2, 0);
		create_png_texture(&text_button[2], button3, 0);
		text_button[3]=text_button[1];

		create_png_texture(& default_game_texture, defpng, 0);
		
		
		for(n=0;n<128*64*3;n++)
			{
			switch((rand()>>3) & 7)
				{
				case 0:
					game_empty[n]=0xfff0f0f0;break;
				case 1:
					game_empty[n]=0xff404040;break;
				case 2:
					game_empty[n]=0xffd0d0d0;break;
				case 3:
					game_empty[n]=0xffc0c0c0;break;
				case 4:
					game_empty[n]=0xffc0a000;break;
				case 5:
					game_empty[n]=0xff404040;break;
				case 6:
					game_empty[n]=0xffd0d0d0;break;
				case 7:
					game_empty[n]=0xffc0c0c0;break;
				}
			}


		CreateTexture(&text_game_empty[0], TILE_SRGBA8, &game_empty[0], 128, 64, 0);
		CreateTexture(&text_game_empty[1], TILE_SRGBA8, &game_empty[128*64], 128, 64, 0);
		CreateTexture(&text_game_empty[2], TILE_SRGBA8, &game_empty[128*64*2], 128, 64, 0);
		text_game_empty[3]=text_game_empty[1];

		SelectFontTexture(1); // selecciona la fuente de letra extra

		letter_size(8,32);
				
		PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
				
		bkcolor=0;
		autocenter=1;
		SetTexture(NULL);

		if(ret!=0) 
			{
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

			s_printf("ERROR: You need cIOS222 and/or cIOS249 to work!!!\n");
			Screen_flip();
			goto error;
			}
		
        
		//ret = WBFS_Init();
		if (ret2 < 0) {
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

			s_printf("ERROR: Could not initialize USB subsystem! (ret = %d)", ret2);

			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 4, 0xa0000000);
			
			PY+=80;
			s_printf("Maybe you need plug the device on USB port 0...");



			Screen_flip();
			goto error;
			}

		letter_size(16,32);
		PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
				
		bkcolor=0;
		autocenter=1;
		SetTexture(NULL);
		#if 1
		/* Get list length */

		WPAD_Init();
		WPAD_SetIdleTimeout(60*5); // 5 minutes 

		WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); // ajusta el formato para acelerometros en todos los wiimotes
		WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH, SCR_HEIGHT);

		#ifdef USE_MODPLAYER
		ASND_Init();
		ASND_Pause(0);
	// inicializa el modplayer en modo loop infinito
		
		
		MODPlay_Init(&mod_track);


		if (MODPlay_SetMOD (&mod_track, lotus3_2 ) < 0 ) // set the MOD song
			{
			MODPlay_Unload (&mod_track);   
			}
		else  
			{
		
			MODPlay_SetVolume( &mod_track, 16,16); // fix the volume to 16 (max 64)
			MODPlay_Start (&mod_track); // Play the MOD
			}
		#endif

get_games:
		
		frames=0;frames2=0;
		load_png=1;
		scroll_mode=0;
		game_mode=0;
		edit_cfg=0;
		cheat_mode=0;
		select_game_bar=0;

		is_favorite=1;
		insert_favorite=0;
		actual_game=0;
		last_game=-1;
		test_favorite=0;

	

		SetTexture(&text_background);
		DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, 0xffa0a0a0);
		letter_size(16,32);
		PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
				
		bkcolor=0;
		autocenter=1;
		SetTexture(NULL);

		ret = WBFS_Open();
		if (ret < 0) {
			
			
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

			s_printf("No WBFS partition found!\n");
			Screen_flip();
			sleep(4);
			if(menu_format()==0) {ret = WBFS_Open();if(ret<0) goto error_w;}
			else goto exit_ok;
			Screen_flip();
			SetTexture(&text_background);
			DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, 0xffa0a0a0);
			letter_size(16,32);
			PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
					
			bkcolor=0;
			autocenter=1;
			SetTexture(NULL);
			}

		cnt=0;buffer=NULL;
		ret = WBFS_GetCount(&cnt);
		if (ret < 0 || cnt==0) {
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

			s_printf("No Game found\n");
			Screen_flip();
			sleep(4);
			//goto error_w;
			}
		else
	       {

			/* Buffer length */
			len = sizeof(struct discHdr) * cnt;

			/* Allocate memory */
			buffer = (struct discHdr *)memalign(32, len);
			if (!buffer){
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("Error in memalign()\n");
				Screen_flip();
				goto error_w;
				}

			/* Clear buffer */
			memset(buffer, 0, len);

			/* Get header list */
			ret = WBFS_GetHeaders(buffer, cnt, sizeof(struct discHdr));
			if (ret < 0) {
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("Error in WBFS_GetHeaders()\n");
				Screen_flip();
				free(buffer);
				goto error_w;
				}
		
		    gameList = buffer;
		    for(n=0;n<cnt;n++)
				{
				  if(gameList[n].id[0]=='_') {memcpy(&gameList[n],&gameList[cnt-1],sizeof(struct discHdr));cnt--;}
				}
		    /* Sort entries */
		    qsort(buffer, cnt, sizeof(struct discHdr), __Menu_EntryCmp);
		    }

		/*
		// quitar 10
		for(n=1;n<10;n++) memcpy(((char *) buffer)+sizeof(struct discHdr) * cnt*n,buffer,sizeof(struct discHdr) * cnt);
		cnt*=10;
		*/
		
		/* Set values */
		gameList = buffer;
		gameCnt  = cnt;

		load_cfg();
		#endif

	autocenter=0;
	


	guitar_pos_x=SCR_WIDTH/2-32;guitar_pos_y=SCR_HEIGHT/2-32;

	
	old_temp_sel=-1;
    
	while(1)
	{
	int temp_sel=-1;
	int test_gfx_page=0;
	int go_home=0;

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//if((frames2 & 255)==0) ASND_SetVoice(1, VOICE_MONO_8BIT, 4096,0, &sound_effects[0][0], 2048/8, 255, 255, NULL);
//	if((frames2 & 255)==0) ASND_SetVoice(1, VOICE_MONO_8BIT, 15000,0, &sound_effects[1][0], 2048, 255, 255, NULL);

	SetTexture(&text_background/*NULL*/);
	set_text_screen_fx();
   
	ConfigureForTexture(10);
	GX_Begin(GX_QUADS,  GX_VTXFMT0, 4);
	AddTextureVertex(0, 0, 999, 0xfff0f0f0, 0, (frames2 & 1023));
	AddTextureVertex(SCR_WIDTH, 0, 999, 0xffa0a0a0, 1023, (frames2 & 1023)); 
	AddTextureVertex(SCR_WIDTH, SCR_HEIGHT, 999, 0xfff0f0f0, 1023, 1024+(frames2 & 1023)); 
	AddTextureVertex(0, SCR_HEIGHT, 999, 0xffa0a0a0, 0, 1024+(frames2 & 1023)); 
	GX_End();
		
    cnt=actual_game;
	SelectFontTexture(1); // selecciona la fuente de letra extra


    {
	int m;
    int ylev=(SCR_HEIGHT-440);

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	

	if(game_mode)
		{
		struct discHdr *header = &gameList[game_datas[game_mode-1].ind];

		SetTexture(NULL);
		DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

		if(game_datas[game_mode-1].png_bmp) 
			SetTexture(&game_datas[game_mode-1].texture);
		else SetTexture(&default_game_texture);

		DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffffffff);
			
		SetTexture(NULL);
			
		DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=ylev-32;/* 8+ylev+2*110*/; color= 0xff000000; 
				
		bkcolor=0;//0xc0f08000;
		
		autocenter=1;
		s_printf("%s",header->title);
		autocenter=0;

		letter_size(16,32);
		
		bkcolor=0;
		if(cheat_mode)
		{

		if(txt_cheats)
			{
			int f=0;
			PX= 26; PY=ylev+16; color= 0xff000000;
			letter_size(16,32);
			autocenter=1;
			bkcolor=0xb0f0f0f0;
			s_printf("%s", &letrero[idioma][10][0]);
			bkcolor=0;
			autocenter=0;
			letter_size(8,32);

			for(n=0;n<5;n++)
				{
				if((actual_cheat+n)>=num_list_cheats) break;
				if(!data_cheats[actual_cheat+n].title) break;
				if(Draw_button2(30+16, ylev+56+56*n, data_cheats[actual_cheat+n].title,data_cheats[actual_cheat+n].apply)) 
					{
					if(select_game_bar!=(500+actual_cheat+n)) scroll_text=-10;
					//if(select_game_bar!=500+actual_cheat+n)  snd_fx_tick();


					select_game_bar=500+actual_cheat+n;
					f=1;
					}
				}
			
			if(f==0) select_game_bar=-1;

			if(num_list_cheats)
					{
					SetTexture(NULL);
					if(actual_cheat>=5)
						{
						if(px>=0 && px<=80 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							DrawFillEllipse(40, ylev+220, 50, 50, 0, 0xc0f0f0f0);
							letter_size(32,64);
							PX= 40-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
							s_printf("-");
							DrawEllipse(40, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
							select_game_bar=-1;
							test_gfx_page=-1;

							if(old_temp_sel!=1000) 
								{
								snd_fx_tick();
								old_temp_sel=1000;
								}
							}
						else
						if(frames2 & 32)
							{
							DrawFillEllipse(40, ylev+220, 40, 40, 0, 0xc0f0f0f0);
							letter_size(32,48);
							PX= 40-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
							s_printf("-");
							DrawEllipse(40, ylev+220, 40, 40, 0, 6, 0xc0000000);
							}
						}

						if((actual_cheat+5)<num_list_cheats)
						{
						if(px>=SCR_WIDTH-82 && px<=SCR_WIDTH-2 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							DrawFillEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 0xc0f0f0f0);
							letter_size(32,64);
							PX= SCR_WIDTH-42-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
							s_printf("+");
							DrawEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
							select_game_bar=-1;
							test_gfx_page=1;

							if(old_temp_sel!=1000) 
								{
								snd_fx_tick();
								old_temp_sel=1000;
								}
							}
						else
						if(frames2 & 32)
							{
							DrawFillEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 0xc0f0f0f0);
							letter_size(32,48);
							PX= SCR_WIDTH-42-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
							s_printf("+");
							DrawEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 6, 0xc0000000);
							}
						}

						if(old_temp_sel>=1000 && test_gfx_page==0) old_temp_sel=-1;
						
					}

			
			}
		else
			{
			PX= 26; PY=ylev+108*2-64; color= 0xff000000;
			letter_size(16,64);
			autocenter=1;
			bkcolor=0xb0f0f0f0;
			s_printf("%s", &letrero[idioma][10][0]);
			bkcolor=0;
			autocenter=0;
			letter_size(16,32);
			}
													
		if(select_game_bar>=500 && select_game_bar<500+num_list_cheats && data_cheats[select_game_bar-500].description)
				{
					PX=40;PY=ylev+108*4-64+16;
					DrawRoundFillBox(20, ylev+108*4-64, 148*4, 56, 0, 0xffcfcf00);
					DrawRoundBox(20, ylev+108*4-64, 148*4, 56, 0, 4, 0xffcf0000);
					letter_size(8,32);
				
					draw_box_text(data_cheats[select_game_bar-500].description);
				
				}
		else
			{
			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][11][0])) select_game_bar=1000;
			if(Draw_button(600-32-strlen(&letrero[idioma][12][0])*8, ylev+108*4-64, &letrero[idioma][12][0])) select_game_bar=1001;

			}

		}
		else
        if(!edit_cfg)
			{
			PX= 26; PY=ylev+12; color= 0xff000000; 
			bkcolor=0xb0f0f0f0;
			if((game_datas[game_mode-1].config & 1) || force_ios249) s_printf("cIOS 249"); else s_printf("cIOS 222");
			
			forcevideo=(game_datas[game_mode-1].config>>2) & 3;if(forcevideo==3) forcevideo=0;

			langsel=(game_datas[game_mode-1].config>>4) & 15;if(langsel>10) langsel=0;

			force_ingame_ios=1*((game_datas[game_mode-1].config>>31)!=0);

			if(forcevideo==1) s_printf(" Forced to PAL");
			if(forcevideo==2) s_printf(" Forced to NTSC");
			
			PX=608-6*16; s_printf("%.2fGB", f_size);
		 
			autocenter=1;
			PX= 26; PY=ylev+352-48;
			if(langsel) s_printf("%s", &languages[langsel][0]);
			autocenter=0;

			bkcolor=0x0;
		
			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;
			if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][1][0])) select_game_bar=2;
			
			if(test_favorite)
				{
				if(is_favorite)
					{if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][2][0])) select_game_bar=4;}
				else
					if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][3][0])) select_game_bar=3;
				}

			

			if(Draw_button(600-32-strlen(&letrero[idioma][4][0])*8, ylev+108*4-64, &letrero[idioma][4][0])) select_game_bar=5;
			}
		else
			{// edit config
			int g;

			PX= 36; PY=ylev+8; color= 0xff000000; 
			letter_size(12,24);
			autocenter=1;
			bkcolor=0xb0f0f0f0;
			s_printf(" Select Language ");
			bkcolor=0;
			autocenter=0;
			
			for(g=0;g<11;g++)
				{
				if((g & 3)==0) m=36+32; else m=x_temp+16;

				if(Draw_button2(m, ylev+36+56*(g/4), &languages[g][0],langsel==g)) select_game_bar=100+g;
				}
			m=x_temp+16;
			if(Draw_button2(m, ylev+36+56*(11/4), &languages[0][0],langsel==0)) select_game_bar=100;

			PY=m=ylev+36+56*(g/4)+56;
			PX=36+192-36;
			letter_size(12,24);
			autocenter=0;
			bkcolor=0xb0f0f0f0;
			s_printf(" Video Mode ");
			PX=460-12;s_printf(" Select cIOS ");
			bkcolor=0;
			m+=28;

			if(Draw_button2(36, m, " Autodetect ",(forcevideo==0))) select_game_bar=200;
			if(Draw_button2(x_temp+12, m, " Force PAL ",(forcevideo==1))) select_game_bar=201;
			if(Draw_button2(x_temp+12, m, " Force NTSC ",(forcevideo==2))) select_game_bar=202;

			if(Draw_button2(472, m, " cIOS 222 ", force_ios249 ? -1 : cios==222)) select_game_bar=300;
			if(Draw_button2(472, m+56, " cIOS 249 ",cios==249)) select_game_bar=301;


			if(Draw_button2(36, m+56, " Skip IOS ", force_ios249 ? -1 : force_ingame_ios!=0)) select_game_bar=302;

			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][8][0])) select_game_bar=10;
			if(Draw_button(600-32-strlen(&letrero[idioma][9][0])*8, ylev+108*4-64, &letrero[idioma][9][0])) select_game_bar=11;

			}
		
		step_button++;
		}
	else
		{ // modo panel
		int set_set=0;
		int selected_panel=-1;
		select_game_bar=0;

		PX= 0; PY=ylev-32; color= 0xff000000; 
				
		bkcolor=0;//0xc0f08000;
		letter_size(16,32);
		autocenter=1;
		
		if(gameList==NULL) is_favorite=0;

		if(is_favorite && !insert_favorite)
			{
			for(n=0;n<16;n++)
				if(config_file.id[n][0]!=0) break;
			if(n==16) 
				{is_favorite=0;
				actual_game=0;
				if(last_game>=0) {actual_game=last_game;last_game=-1;}
				}
			}
		if(is_favorite) 
			{
			if(insert_favorite) s_printf("%s", &letrero[idioma][5][0]);
				else s_printf("%s", &letrero[idioma][6][0]);
			}
		else
			s_printf("%s %i",&letrero[idioma][7][0],1+(actual_game/16));

		autocenter=0;
		bkcolor=0;
  
		for(m=0;m<4;m++)
		for(n=0;n<4;n++)
			{
			

			if(gameList==NULL) cnt=gameCnt+10;
			else
			if(is_favorite) 
				{
				int g;
				
				if(config_file.id[(m<<2)+n][0]==0)
					{
					cnt=gameCnt+1;
					}
				else
					{
					cnt=gameCnt+1;
					for(g=0;g<gameCnt;g++)
						{
						struct discHdr *header = &gameList[g];
						if(strncmp((char *) header->id, (char *) &config_file.id[(m<<2)+n][0],6)==0)
							{
							cnt=g;break;
							}
						}
					}

				}

			//if(cnt>=gameCnt) cnt=0;

			if(cnt<gameCnt)
				{
				struct discHdr *header = &gameList[cnt];

				game_datas[(m<<2)+n].ind=cnt;
				
				memcpy(discid,header->id,6); discid[6]=0;

				if(load_png)
					{
					memset(temp_data,0,256*1024);
					WBFS_GetProfileDatas(discid, temp_data);
					create_game_png_texture((m<<2)+n);
					}
				letter_size(8,24);
				
				if(!(px>=20+n*150 && px<20+n*150+148 && py>=ylev+m*110 && py<ylev+m*110+108))
					{
					PX= 26+n*150+scroll_mode; PY= 64-24+30+8+ylev+m*110; color= 0xff000000; 
					
					bkcolor=0xc0f0f000;

					s_printf_z=4;
					if(!game_datas[(m<<2)+n].png_bmp) 
									draw_text(header->title);
					s_printf_z=0;
					bkcolor=0;
					}
				else if(scroll_mode==0)
					{
					selected_panel=(m<<2)+n;
		
					}

				SetTexture(NULL);
				DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffafafaf);

				if(game_datas[(m<<2)+n].png_bmp) 
					SetTexture(&game_datas[(m<<2)+n].texture);
				else SetTexture(&default_game_texture);
				
				
				DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffffffff);

				SetTexture(&text_screen_fx);
				DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffffffff);
				SetTexture(NULL);

				
				set_set=1;

				cnt++;
				}
			else
				{	
				set_set=0;
				//SetTexture(STRIPED_PATTERN);
				game_datas[(m<<2)+n].ind=-1;
				SetTexture(&text_game_empty[(frames2>>2) & 3]);
				DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffffffff);

				SetTexture(&text_screen_fx);
				DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffffffff);
				SetTexture(NULL);
				}
			
			SetTexture(NULL);
			if(px>=20+n*150 && px<20+n*150+148 && py>=ylev+m*110 && py<ylev+m*110+108)
				{
				
				if(old_temp_sel!=(m<<2)+n && old_temp_sel<1000) 
					{
					snd_fx_tick();
					old_temp_sel=(m<<2)+n;
					}
				if(set_set || insert_favorite) temp_sel=(m<<2)+n; else temp_sel=-1;
				DrawRoundBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 6, 0xfff08000/*0xff00a0a0*/);
				}
			else
				DrawRoundBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 4, 0xff303030);

			// scroll mode
				if(scroll_mode<0)
					{
					SetTexture(&text_game_empty[(frames2>>2) & 3]);
					DrawRoundFillBox(20+n*150+scroll_mode+620, ylev+m*110, 148, 108, 0, 0xffffffff);
					SetTexture(NULL);
					DrawRoundBox(20+n*150+scroll_mode+620, ylev+m*110, 148, 108, 0, 4, 0xff303030);
					}
				if(scroll_mode>0)
					{
					SetTexture(&text_game_empty[(frames2>>2) & 3]);
					
					DrawRoundFillBox(20+n*150+scroll_mode-620, ylev+m*110, 148, 108, 0, 0xffffffff);
					SetTexture(NULL);
					DrawRoundBox(20+n*150+scroll_mode-620, ylev+m*110, 148, 108, 0, 4, 0xff303030);
					}

				if(!scroll_mode && !insert_favorite)
					{
					SetTexture(NULL);
			
						if(px>=0 && px<=80 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							DrawFillEllipse(40, ylev+220, 50, 50, 0, 0xc0f0f0f0);
							letter_size(32,64);
							PX= 40-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
							s_printf("-");
							DrawEllipse(40, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
							temp_sel=-1;
							test_gfx_page=-1;

							if(old_temp_sel!=1000) 
								{
								snd_fx_tick();
								old_temp_sel=1000;
								}
							}
						else
						if(frames2 & 32)
							{
							DrawFillEllipse(40, ylev+220, 40, 40, 0, 0xc0f0f0f0);
							letter_size(32,48);
							PX= 40-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
							s_printf("-");
							DrawEllipse(40, ylev+220, 40, 40, 0, 6, 0xc0000000);
							}
						

					
						if(px>=SCR_WIDTH-82 && px<=SCR_WIDTH-2 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							DrawFillEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 0xc0f0f0f0);
							letter_size(32,64);
							PX= SCR_WIDTH-42-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
							s_printf("+");
							DrawEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
							temp_sel=-1;
							test_gfx_page=1;

							if(old_temp_sel!=1001) 
								{
								snd_fx_tick();
								old_temp_sel=1001;
								}
							}
						else
						if(frames2 & 32)
							{
							DrawFillEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 0xc0f0f0f0);
							letter_size(32,48);
							PX= SCR_WIDTH-42-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
							s_printf("+");
							DrawEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 6, 0xc0000000);

							}

						if(old_temp_sel>=1000 && test_gfx_page==0) old_temp_sel=-1;
						
					}
			}

			if(selected_panel>=0 && scroll_mode==0)
				{struct discHdr *header = &gameList[game_datas[selected_panel].ind];
		
					SetTexture(NULL);
					DrawRoundFillBox(20, ylev+ ((selected_panel>=12) ? 0 : 3*110), 150*4, 108, 0, 0xcfcfffff);
					DrawRoundBox(20, ylev+ ((selected_panel>=12) ? 0 : 3*110), 150*4, 108, 0, 6, 0xcff08000);
		
					if(strlen(header->title)<=37) letter_size(16,32);
					else if(strlen(header->title)<=45) letter_size(12,32);
					else letter_size(8,32);		

					PX= 0; PY=ylev+ ((selected_panel>=12) ? 0 : 3*110)+(108-32)/2; color= 0xff000000; 
							
					bkcolor=0;
					
					autocenter=1;
					s_printf("%s",header->title);
					autocenter=0;
				}

		
		
		} // modo panel
		} 

	load_png=0;
	frames2++;
	
	
		{
		if(select_game_bar>=0)
			{
			if(select_game_bar!=last_select)
				{
				snd_fx_tick();
				last_select=select_game_bar;
				}
			}
		else last_select=-1;
		}

	if(!scroll_mode)
		{
		frames++;
		if(frames>=8) {frames=0;if(txt_cheats) scroll_text+=2; else scroll_text++;}

		temp_pad= wiimote_read(); 
		new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

		if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
						if(insert_favorite)
							{

							SetTexture(&text_move_chan);

							DrawRoundFillBox(px-148/2, py-108/2, 148, 108, 0, 0x8060af60);
							
							}
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(px<16) px=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}
						
						px=guitar_pos_x; py=guitar_pos_y;

						if(insert_favorite)
							{
							SetTexture(&text_move_chan);
							DrawRoundFillBox(px-148/2, py-108/2, 148, 108, 0, 0x8060af60);
							}
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						
						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) new_pad|=WPAD_BUTTON_A;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) old_pad|=WPAD_BUTTON_A;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) new_pad|=WPAD_BUTTON_1;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) old_pad|=WPAD_BUTTON_1;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) new_pad|=WPAD_BUTTON_MINUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) old_pad|=WPAD_BUTTON_MINUS;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) new_pad|=WPAD_BUTTON_PLUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) old_pad|=WPAD_BUTTON_PLUS;

						}
					 else
					   {px=-100;py=-100;}

					 if(cheat_mode && txt_cheats)
						{
						if(new_pad & WPAD_BUTTON_MINUS)
							{
								actual_cheat-=5;if(actual_cheat<0) actual_cheat=0;
							}
						
						if(new_pad & WPAD_BUTTON_PLUS)
							{
							if((actual_cheat+5)<num_list_cheats) actual_cheat+=5;
							}
						}

					 if(!game_mode && !insert_favorite && !cheat_mode && gameList!=NULL) //limit left/right
						{
						
						if(new_pad & WPAD_BUTTON_MINUS)
							{

								scroll_mode=1;
								px=py=-100;
							}
						if(new_pad & WPAD_BUTTON_PLUS)
							{
								scroll_mode=-1;
								px=py=-100;
							
							}
						} // limit left_right


					if((new_pad & WPAD_BUTTON_A) && gameList!=NULL)
						{
						if(test_gfx_page)
							{
							 if(cheat_mode && txt_cheats)
								{
								if(test_gfx_page<0)
									{
										actual_cheat-=5;if(actual_cheat<0) actual_cheat=0;
										snd_fx_yes();
									}
								if(test_gfx_page>0)
									{
										actual_cheat+=5;if(actual_cheat>=num_list_cheats) actual_cheat=num_list_cheats-5;
										snd_fx_yes();
									}
								}
							else
							if(insert_favorite==0)
								{
								if(gameList!=NULL)
									{
									if(test_gfx_page<0)
										{scroll_mode=1;px=py=-100;snd_fx_yes();}
									if(test_gfx_page>0)
										{scroll_mode=-1;px=py=-100;snd_fx_yes();}
									}
								}
							}
						else
						if(game_mode==0)
							{
							if((old_pad & WPAD_BUTTON_B) && is_favorite)
								{
								if(temp_sel>=0)
									{
									insert_favorite=game_datas[temp_sel].ind+1;
									snd_fx_yes();

									mem_move_chan=NULL;

									if(game_datas[temp_sel].png_bmp) 
										memcpy(&text_move_chan, &game_datas[temp_sel].texture, sizeof(GXTexObj));
									else
									memcpy(&text_move_chan, &default_game_texture, sizeof(GXTexObj));

									
									}
								}
							else
								{
								// insert game in favorites
								if(insert_favorite)
									{
								
									struct discHdr *header = &gameList[insert_favorite-1];
									int n,m=0;

									if(temp_sel>=0)
										{
										for(n=0;n<16;n++)
											{
											if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
												{
												if(m==0)
													{m=1;memcpy(&config_file.id[n][0],&config_file.id[temp_sel][0],8);config_file.id[n][6]=0;memset(&config_file.id[temp_sel][0],0,8);}
												else {memset(&config_file.id[n][0],0,8);} // elimina repeticiones

												break;
												}
											}
										snd_fx_yes();
										memcpy(&config_file.id[temp_sel][0],header->id,6);
										config_file.id[temp_sel][6]=0;
										insert_favorite=0;if(mem_move_chan) free(mem_move_chan);mem_move_chan=NULL;
										temp_sel=-1;
										for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
										save_cfg();
										load_png=1;
										}
									}
								else
									{
									int n;
									struct discHdr *header;
									if(!is_favorite) last_game=actual_game;
									
									if(temp_sel>=0)
									     snd_fx_yes();
									else snd_fx_no();

									game_mode=temp_sel+1;

									header = &gameList[game_datas[game_mode-1].ind];
									
									if(WBFS_GameSize(header->id, &f_size)) f_size=0.0f;

									if(is_favorite) test_favorite=0; else test_favorite=1;
									
									for(n=0;n<16;n++)
										{
										if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
											{
											if(!is_favorite) test_favorite=0; else test_favorite=1;
											break;
											}
										}
								

									}
								}
							}
						else
							{
							if(select_game_bar==1) {int n;  // return
													snd_fx_no();
													game_mode=0;edit_cfg=0;select_game_bar=0;
													for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   
													load_png=1;
													} 

							if(select_game_bar==2) 
												  { // Configuration (load)
							                       struct discHdr *header = &gameList[game_datas[game_mode-1].ind];

												   memcpy(discid,header->id,6); discid[6]=0;

												   snd_fx_yes();

												   select_game_bar=0;

												   memset(temp_data,0,256*1024);
												   WBFS_GetProfileDatas(discid, temp_data);
												   game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);
												   
												   if((game_datas[game_mode-1].config & 1) || force_ios249) cios=249; else cios=222;

												   forcevideo=(game_datas[game_mode-1].config>>2) & 3;if(forcevideo==3) forcevideo=0;

												   langsel=(game_datas[game_mode-1].config>>4) & 15;if(langsel>10) langsel=0;

												   force_ingame_ios=1*((game_datas[game_mode-1].config>>31)!=0);

												   edit_cfg=1;
												   }

							if(select_game_bar==3) {int n;// add favorite
													select_game_bar=0;is_favorite=1;insert_favorite=game_datas[game_mode-1].ind+1;

													snd_fx_yes();

													mem_move_chan=game_datas[game_mode-1].png_bmp;game_datas[game_mode-1].png_bmp=NULL;

													if(game_datas[game_mode-1].png_bmp) 
														memcpy(&text_move_chan, &game_datas[game_mode-1].texture, sizeof(GXTexObj));
													else
														memcpy(&text_move_chan, &default_game_texture, sizeof(GXTexObj));
													
													game_mode=0;edit_cfg=0;
													
												    for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												    save_cfg();
												    load_png=1;
													} 
							if(select_game_bar==4) {// del favorite
												   int n;
												   select_game_bar=0;is_favorite=1; 

												   snd_fx_yes();

												   memset(&config_file.id[game_mode-1][0],0,8);
												   game_mode=0;edit_cfg=0;
												   for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   save_cfg();
												   load_png=1;
												   }
							if(select_game_bar>=500 && select_game_bar<500+MAX_LIST_CHEATS)
													{
													snd_fx_yes();
													data_cheats[select_game_bar-500].apply^=1;
													}
							if(select_game_bar==5 || select_game_bar==1000 || select_game_bar==1001) 
													{
													struct discHdr *header = &gameList[game_datas[game_mode-1].ind];

													snd_fx_yes();

													memcpy(discid,header->id,6); discid[6]=0;
													
													if(select_game_bar==5)
														if(load_cheats(discid)) cheat_mode=1;

													if(select_game_bar>=1000) cheat_mode=0;

													if(select_game_bar==1000) create_cheats();
													
													if(select_game_bar==1001) len_cheats=0; // don't apply cheats
						
													select_game_bar=0;

													if(!cheat_mode)
														{
														select_game_bar=0;
														Screen_flip();
														WPAD_Shutdown();
														
														save_cfg();
														
														
														if((game_datas[game_mode-1].config & 1) || force_ios249) cios=249; else cios=222;

														forcevideo=(game_datas[game_mode-1].config>>2) & 3;if(forcevideo==3) forcevideo=0;

														langsel=(game_datas[game_mode-1].config>>4) & 15;if(langsel>10) langsel=0;

														force_ingame_ios=1*((game_datas[game_mode-1].config>>31)!=0);
														
														fatUnmount("sd");
														__io_wiisd.shutdown();

														USBStorage2_Deinit();
														WDVD_Close();
														
														#ifdef USE_MODPLAYER
														ASND_End();		// finaliza el sonido
														#endif
														if(cios!=222)
															{
															IOS_ReloadIOS(cios);
															sleep(1);
															load_ehc_module();
															for(n=0;n<25;n++)
																{
																	ret2 = USBStorage2_Init(); 
																	if(!ret2) break;
																	usleep(200*1000);
																}
															USBStorage2_Deinit();
															}

														// enables fake ES_ioctlv (to skip IOS_ReloadIOS(ios))
														if(force_ingame_ios)
															{
															patch_datas[0]=*((u32 *) (dip_plugin+16*4));
															mload_set_ES_ioctlv_vector((void *) patch_datas[0]);
															}
															
														mload_close();

														ret = load_disc(discid);

														//////////////////////////////////
													
														SetTexture(&text_background);
														DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, 0xffa0a0a0);
			
												
														SelectFontTexture(1); // selecciona la fuente de letra extra

														letter_size(8,32);
																
														PX=0; PY= SCR_HEIGHT/2; color= 0xff000000; bkcolor=0;
																
														bkcolor=0;
														autocenter=1;
														SetTexture(NULL);

														DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
														DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);
														
														if(ret==666)s_printf("ERROR FROM THE LOADER: Disc ID is not equal!\n"); 
														else 
															s_printf("ERROR FROM THE LOADER: %i\n", ret);

														Screen_flip();


														//////////////////////////////////
														break;
														}
												   } // load game

							if(select_game_bar==10 || select_game_bar==11) { // return from config (saving)
												   edit_cfg=0;

												   if(select_game_bar==10) snd_fx_yes(); else snd_fx_no();
												   // si no existe crea uno
												   if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
														{
														temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=0;
														temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
														temp_data[8]=temp_data[9]=temp_data[10]=temp_data[11]=0;
														}
													
													
												   temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
												   if(select_game_bar==11) 
														{
													    temp_data[4]=game_datas[game_mode-1].config & 255;
														temp_data[5]=(game_datas[game_mode-1].config>>8) & 255;
														temp_data[6]=(game_datas[game_mode-1].config>>16) & 255;
														temp_data[7]=(game_datas[game_mode-1].config>>24) & 255;
														}
												   else
														{
														if(cios==249) temp_data[4]|=1;
														temp_data[4]|=(forcevideo & 3)<<2;
														temp_data[4]|=(langsel & 15)<<4;
														temp_data[7]|=((force_ingame_ios!=0) & 1)<<7;
														}

												   game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);

												   WBFS_SetProfileDatas(discid, temp_data);
												   
												   }
							if(select_game_bar>=100 && select_game_bar<=111)
												   {
												   langsel= select_game_bar-100;
												   snd_fx_yes();
												   }
							if(select_game_bar>=200 && select_game_bar<=202)
												   {
												   forcevideo= select_game_bar-200;
												   snd_fx_yes();
												   }
							if(select_game_bar>=300 && select_game_bar<=301)
												   {
												   cios= (select_game_bar-300) ? 249 : 222;
												   snd_fx_yes();
												   }
							if(select_game_bar==302)
												   {
												   force_ingame_ios=(force_ingame_ios==0);
												   snd_fx_yes();
												   }
												   

							}
						
						}

					
					
					if(new_pad & WPAD_BUTTON_B)
						{
						
						int n;
						
						if(cheat_mode)
							{
							}
						else
						if(edit_cfg)
							{// return from config (saving)
							edit_cfg=0;
							snd_fx_yes();
						    // si no existe crea uno
						    if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
								{
								temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=0;
								temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
								temp_data[8]=temp_data[9]=temp_data[10]=temp_data[11]=0;
								}
							
							
						    temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
						    if(cios==249) temp_data[4]|=1;
						    temp_data[4]|=(forcevideo & 3)<<2;
						    temp_data[4]|=(langsel & 15)<<4;
							temp_data[7]|=((force_ingame_ios!=0) & 1)<<7;

						    game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);

						    WBFS_SetProfileDatas(discid, temp_data);
							}
						else
							{
							
							if(game_mode)
								{
								snd_fx_no();
								for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
													   
								load_png=1;
								}
							else
								if(insert_favorite) snd_fx_no();

							game_mode=0;edit_cfg=0;
							insert_favorite=0;if(mem_move_chan) free(mem_move_chan);mem_move_chan=NULL;
							}
						}

					if(new_pad & WPAD_BUTTON_1) // swap favorites/page
						{
						int n;

						scroll_text=-20;

						if(!game_mode && !insert_favorite)
							{
							if(is_favorite)
								{
								is_favorite=0;if(last_game>=0) {actual_game=last_game;last_game=-1;}
								}
							else
								{
								last_game=actual_game;is_favorite=1;
								}
							for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   
							load_png=1;
							}
						
						}
					if((new_pad & WPAD_BUTTON_HOME) && game_mode==0 && insert_favorite==0 && scroll_mode==0) go_home=1;
							
			
			} 
			else {px=-100;py=-100;}
		}
	else // scroll mode
		{
		if(scroll_mode<0) scroll_mode-=32;
		if(scroll_mode>0) scroll_mode+=32;

		if((scroll_mode & 64)==0) snd_fx_scroll();

		if(scroll_mode<-639 || scroll_mode>639)
			{
			int n,g;
			if(scroll_mode<0)
				{
				if(is_favorite) {is_favorite=0;actual_game=0;if(last_game>=0) {actual_game=last_game;last_game=-1;}} 
				else {actual_game+=16;if(actual_game>=gameCnt) {actual_game=0;is_favorite=1;}}
				}
			if(scroll_mode>0)
				{
				
				if(!is_favorite)
					{
					actual_game-=16; 
				    if(actual_game<0) 
						{
						int flag=0;
						last_game=-1;is_favorite=1;actual_game=0;
					    for(n=0;n<16;n++)
							if(config_file.id[n][0]!=0)	
								{
								for(g=0;g<gameCnt;g++)
									{
									struct discHdr *header = &gameList[g];
									if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
										{
										flag=1;break;
										}
									 }
								}
						if(!flag) {actual_game=((gameCnt-1)/16)*16;is_favorite=0;}
						}
					}
					else {actual_game=((gameCnt-1)/16)*16;is_favorite=0;}
				}

			scroll_mode=0;
			scroll_text=-20;

			for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
			load_png=1;

			
			}
		}

	if(load_png)
			{
			SelectFontTexture(0); // selecciona la fuente de letra normal

			letter_size(32,64);
															
			PX=0; PY= SCR_HEIGHT/2-32; color= 0xcf000000; bkcolor=0;
															
			autocenter=1;
			SetTexture(NULL);
			DrawRoundFillBox((SCR_WIDTH-320)/2, SCR_HEIGHT/2-40, 320, 80, 0, 0xcf00ff00);
			DrawRoundBox((SCR_WIDTH-320)/2, SCR_HEIGHT/2-40, 320, 80, 0, 4, 0xcf000000);

			s_printf("Loading");
			autocenter=0;
			Screen_flip();


			SelectFontTexture(0); // selecciona la fuente de letra extra
			}

	Screen_flip();

	if(go_home)
		{
		struct discHdr *header =NULL;
		if(temp_sel>=0)
			{//
			int g;
			/*if(is_favorite) 
				{
				
				if(config_file.id[temp_sel][0]!=0)
					{
					id=&config_file.id[temp_sel][0]
					}
				}
			else
				{
				}*/

			g=game_datas[temp_sel].ind;
			if(g>=0)
				{
				header = &gameList[g];
				}

			}//

		n=home_menu(header);
		if(n==1) {exit_by_reset=return_reset;break;}
		if(n==2) {Screen_flip();goto get_games;}
		if(n==3) load_png=1;
		go_home=0;
		}

	if(exit_by_reset)  
		{	
		break;
		}

	}// while
	exit_ok:

	mload_set_ES_ioctlv_vector(NULL);
	mload_close();
	#ifdef USE_MODPLAYER
		ASND_End();		// finaliza el sonido
	#endif
	WPAD_Shutdown();
	sleep(1);

	if(exit_by_reset==2)
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	if(exit_by_reset==3)
		SYS_ResetSystem(SYS_POWEROFF_STANDBY, 0, 0);
	return 0;
error_w:

	mload_set_ES_ioctlv_vector(NULL);
	mload_close();
	 #ifdef USE_MODPLAYER
		ASND_End();		// finaliza el sonido
	#endif
	WPAD_Shutdown();

error:
    sleep(4);

	if(return_reset==2)
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);

    return ret;
}


#include <ogc/lwp_watchdog.h>
extern void settime(long long);

int load_disc(u8 *discid)
{
        static struct DiscHeader Header ALIGNED(0x20);
        static struct Partition_Descriptor Descriptor ALIGNED(0x20);
        static struct Partition_Info Partition_Info ALIGNED(0x20);
        int i;

        memset(&Header, 0, sizeof(Header));
        memset(&Descriptor, 0, sizeof(Descriptor));
        memset(&Partition_Info, 0, sizeof(Partition_Info));

		
		/*
		// fix speed to 6x (MAX)
		for(i=0;i<5;i++)
			{
			if(!USBStorage2_Init()) break;
			}
	
		USBStorage2_WFS_Speed_Fix(0x1);
		USBStorage2_Deinit();
		*/

		WDVD_Init();
        if(WDVD_SetUSBMode(discid)!=0) return 1;

        WDVD_Reset();
		//return -456;
		
        memset(Disc_ID, 0, 0x20);
        WDVD_ReadDiskId(Disc_ID);
		

        if (*Disc_ID==0x10001 || *Disc_ID==0x10001)
                return 2;
      
        Determine_VideoMode(*Disc_Region);
        WDVD_UnencryptedRead(&Header, sizeof(Header), 0);


		if(strncmp((char *) &Header, (char *) discid, 6)) 
		    return 666; // if headerid != discid (on hdd) error
		

        u32 Offset = 0x00040000; // Offset into disc to partition descriptor
        WDVD_UnencryptedRead(&Descriptor, sizeof(Descriptor), Offset);

        Offset = Descriptor.Primary_Offset << 2;

        u32 PartSize = sizeof(Partition_Info);
        u32 BufferLen = Descriptor.Primary_Count * PartSize;
        
        // Length must be multiple of 0x20
        BufferLen += 0x20 - (BufferLen % 0x20);
        u8 *PartBuffer = (u8*)memalign(0x20, BufferLen);

        memset(PartBuffer, 0, BufferLen);
        WDVD_UnencryptedRead(PartBuffer, BufferLen, Offset);

        struct Partition_Info *Partitions = (struct Partition_Info*)PartBuffer;
        for ( i = 0; i < Descriptor.Primary_Count; i++)
        {
                if (Partitions[i].Type == 0)
                {
                        memcpy(&Partition_Info, PartBuffer + (i * PartSize), PartSize);
                        break;
                }
        }
        Offset = Partition_Info.Offset << 2;
        free(PartBuffer);
        if (!Offset)
                return 3;

        WDVD_Seek(Offset);
        Offset = 0;
          
        signed_blob* Certs		= 0;
        signed_blob* Ticket		= 0;
        signed_blob* Tmd		= 0;
        
        unsigned int C_Length	= 0;
        unsigned int T_Length	= 0;
        unsigned int MD_Length	= 0;
        
        static u8	Ticket_Buffer[0x800] ALIGNED(32);
        static u8	Tmd_Buffer[0x49e4] ALIGNED(32);
        
        GetCerts(&Certs, &C_Length);
        WDVD_UnencryptedRead(Ticket_Buffer, 0x800, Partition_Info.Offset << 2);
        Ticket		= (signed_blob*)(Ticket_Buffer);
        T_Length	= SIGNED_TIK_SIZE(Ticket);

		

        // Open Partition and get the TMD buffer
       
		if (WDVD_OpenPartition((u64) Partition_Info.Offset, 0,0,0, Tmd_Buffer) != 0)
                return 4;
        Tmd = (signed_blob*)(Tmd_Buffer);
        MD_Length = SIGNED_TMD_SIZE(Tmd);
        static struct AppLoaderHeader Loader ALIGNED(32);
        WDVD_Read(&Loader, sizeof(Loader), 0x00002440);// Offset into the partition to apploader header
        DCFlushRange((void*)(((u32)&Loader) + 0x20),Loader.Size + Loader.Trailer_Size);


        // Read apploader from 0x2460
        WDVD_Read(Apploader, Loader.Size + Loader.Trailer_Size, 0x00002440 + 0x20);
        DCFlushRange((void*)(((int)&Loader) + 0x20),Loader.Size + Loader.Trailer_Size);


        AppLoaderStart	Start	= Loader.Entry_Point;
        AppLoaderEnter	Enter	= 0;
        AppLoaderLoad		Load	= 0;
        AppLoaderExit		Exit	= 0;
        Start(&Enter, &Load, &Exit);

        void*	Address = 0;
        int		Section_Size;
        int		Partition_Offset;

		switch(langsel)
                {
                        case 0:
                                configbytes[0] = 0xCD;
                        break;

                        case 1:
                                configbytes[0] = 0x00;
                        break;

                        case 2:
                                configbytes[0] = 0x01;
                        break;

                        case 3:
                                configbytes[0] = 0x02;
                        break;

                        case 4:
                                configbytes[0] = 0x03;
                        break;

                        case 5:
                                configbytes[0] = 0x04;
                        break;

                        case 6:
                                configbytes[0] = 0x05;
                        break;

                        case 7:
                                configbytes[0] = 0x06;
                        break;

                        case 8:
                                configbytes[0] = 0x07;
                        break;

                        case 9:
                                configbytes[0] = 0x08;
                        break;

                        case 10:
                                configbytes[0] = 0x09;
                        break;
                }

	
   
		hooktype = 0;
		
		if(len_cheats)
			{ 
			/*HOOKS STUFF - FISHEARS*/
			memset((void*)0x80001800,0,kenobiwii_size);
			memcpy((void*)0x80001800,kenobiwii,kenobiwii_size);
			memcpy((void*)0x80001800, (char*)0x80000000, 6);	// For WiiRD
			DCFlushRange((void*)0x80001800,kenobiwii_size);
			memcpy((void*)0x800027E8, buff_cheats, len_cheats);
            *(vu8*)0x80001807 = 0x01;
			hooktype = 1;
			}
        //printf("Loading game");
        while (Load(&Address, &Section_Size, &Partition_Offset))
        {
                if (!Address) return 5;
                WDVD_Read(Address, Section_Size, Partition_Offset << 2);
                
				/*HOOKS STUFF - FISHEARS*/
				dogamehooks(Address, Section_Size);

				/*LANGUAGE PATCH - FISHEARS*/
				langpatcher(Address, Section_Size);

				vidolpatcher(Address, Section_Size);
				/*HOOKS STUFF - FISHEARS*/

				DCFlushRange(Address, Section_Size);
                //printf(".");
        }
        // Patch in info missing from apploader reads
        *Sys_Magic	= 0x0d15ea5e;
        *Version	= 1;
        *Arena_L	= 0x00000000;
        *Bus_Speed	= 0x0E7BE2C0;
        *CPU_Speed	= 0x2B73A840;

        // Enable online mode in games
        memcpy(Online_Check, Disc_ID, 4);

		
        
        // Retrieve application entry point
        void* Entry = Exit();

        // Set Video Mode based on Configuration
		if (vmode)
			Set_VideoMode();
		

		

        // Flush application memory range
        DCFlushRange((void*)0x80000000, 0x17fffff);	// TODO: Remove these hardcoded values

        // Cleanup loader information
        WDVD_Close();

		#if 1
        // Identify as the game
        if (IS_VALID_SIGNATURE(Certs) 	&& IS_VALID_SIGNATURE(Tmd) 	&& IS_VALID_SIGNATURE(Ticket) 
            &&  C_Length > 0 				&& MD_Length > 0 			&& T_Length > 0)
        {
                int ret = ES_Identify(Certs, C_Length, Tmd, MD_Length, Ticket, T_Length, NULL);
                if (ret < 0)
                        return ret;
        }
		#endif

       // debug_printf("start %p\n",Entry);
	   settime(secs_to_ticks(time(NULL) - 946684800));

       SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

        __asm__ __volatile__
                (
                        "mtlr %0;"			// Move the entry point into link register
                        "blr"				// Branch to address in link register
                        :					// No output registers
                        :	"r" (Entry)		// Input register
                                //:					// difference between C and cpp mode??
                        );
	return 0;
}



char buff_rcheat[256];

int gettype_line(u8 *b, u8 *v, int *nv)
{

int n;

while(*b==32 || *b==10) b++;

if(*b==0) return 0; // linea separadora

for(n=0;n<8;n++)
	{
	if((b[n]>='0' &&  b[n]<='9') || (b[n]>='A' &&  b[n]<='F') || (b[n]>='a' &&  b[n]<='f'))
		{
		v[n>>1]<<=4; v[n>>1]|=(b[n])-48-7*(b[n]>='A')-41*(b[n]>='a');
		}
	else return 1; // cadena
	}

b+=8;

*nv=1;
while(*b==32 || *b==10) b++;
if(*b==0) return 2; // numero

for(n=0;n<8;n++)
	{
	if((b[n]>='0' &&  b[n]<='9') || (b[n]>='A' &&  b[n]<='F') || (b[n]>='a' &&  b[n]<='f'))
		{
		v[4+(n>>1)]<<=4; v[4+(n>>1)]|=(b[n])-48-7*(b[n]>='A')-41*(b[n]>='a');
		}
	else return -1; // error en numero
	}

*nv=2;
return 2; // numero
}

void create_cheats()
{
int n,m,f;

u8 data_head[8] = {
	0x00, 0xD0, 0xC0, 0xDE, 0x00, 0xD0, 0xC0, 0xDE
};
u8 data_end[8] = {
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

if(!txt_cheats) return;

	len_cheats=0;
	buff_cheats =  malloc (16384);
	if(buff_cheats == 0){
		return;
	}

	f=0;
	memcpy(buff_cheats,data_head,8);
	m=0;
	for(n=0;n<MAX_LIST_CHEATS;n++)
		{
		if(data_cheats[n].title && data_cheats[n].apply)
			{
			if(f==0) m+=8;f=1;
			memcpy(buff_cheats+m,data_cheats[n].values,data_cheats[n].len_values);
			m+=data_cheats[n].len_values;
			}
		}
	if(f)
		{
		memcpy(buff_cheats+m,data_end,8);m+=8;
		len_cheats=m;
		}


}

int load_cheats(u8 *discid)
{
char file_cheats[]="sd:/codes/000000.txt";

u8 data_readed[8] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

u8 sub_head[32]="";
u8 temp_sub_head[32]="";

FILE *fp;
int ret;
int n,m;
int mode=0;

if(!sd_ok) return 0;

actual_cheat=0;
num_list_cheats=0;
memcpy(&file_cheats[10],(char *) discid, 6);
len_cheats=0;

txt_cheats=1;
memset(data_cheats,0,sizeof(struct _data_cheats)*MAX_LIST_CHEATS);
fp = fopen(file_cheats, "rb");
	if (!fp) {
		txt_cheats=0;
		file_cheats[17]='g';file_cheats[18]='c';file_cheats[19]='t';
		fp = fopen(file_cheats, "rb");
		
	}

	if (!fp) {
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	len_cheats = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	buff_cheats =  malloc (16384);
	if(buff_cheats == 0){
		fclose(fp);
		return 0;
	}

	if(txt_cheats)
		{
		
		n=m=0;
		mode=0;

		while(1)
			{
			int force_exit=0;
		
			if(fgets(buff_rcheat,256,fp)) 
				{
				int g=0,ret, ndatas;
				while(buff_rcheat[g]!=0 && buff_rcheat[g]!=10 && buff_rcheat[g]!=13) g++;
				buff_rcheat[g]=0;

				ret=gettype_line((u8 *) buff_rcheat, data_readed, &ndatas);

				switch(mode)
					{
					case 0: // get ID
						if(ret==1)
							{
							if(strncmp((char *) discid, (char *) buff_rcheat,6)!=0) force_exit=1; // error! c�digo no coincide
							mode++;
							}
						break;

					case 1: // get name
						if(ret==0)
							mode++;
						break;

					case 2: // get entry name
						if(ret!=0)
							{
							int sub;
							if(ret!=1) force_exit=1;

							memcpy(temp_sub_head, buff_rcheat, (g>31) ? 31 : g);temp_sub_head[(g>31) ? 31 : g]=0;

							sub=strlen((char *) sub_head);

							if(g>(63-sub)) 
								{
								if(g>39) 
									g=39;
								if((63-g)<sub) sub=63-g;
								}

							memset(buff_cheats+m,32,63); // rellena de espacios
							buff_cheats[m+63]=0;
							
							
                            memcpy(buff_cheats+m+(63-(g+sub))/2,sub_head,sub);// centra nombre
							if(sub>0) buff_cheats[m+(63-(g+sub))/2+sub-1]='>';
							memcpy(buff_cheats+m+(63-(g+sub))/2+sub,buff_rcheat,g);// centra nombre
							g=63;

							data_cheats[n].title=buff_cheats+m;
							m+=g+1;
							data_cheats[n].values=buff_cheats+m;
							data_cheats[n].len_values=0;
							data_cheats[n].description=NULL;
							mode++;
							}
						break;
					case 3: // get entry codes
					case 4:
					    if(ret==0 || ret==1) 
							{
						    if(mode==4) 
								{	
								if(ret==1)
									{
									memcpy(buff_cheats+m,buff_rcheat,g+1);
									data_cheats[n].description=buff_cheats+m;
									m+=g+1;
									}
								n++; 
								}
							else 
								{
								int sub;
								if(data_cheats[n].title)
								  {memcpy(sub_head,temp_sub_head,31);sub_head[31]=0;
								   sub=strlen((char *) sub_head); if(sub<31) {sub_head[sub]='>';sub_head[sub+1]=0;}
								   data_cheats[n].title=NULL;}
							    }
							mode=2;if(n>=MAX_LIST_CHEATS) force_exit=1;
							}
						else
						if(ret==2)
							{
							memcpy(buff_cheats+m,data_readed,ndatas*4);
							data_cheats[n].len_values+=ndatas*4;
							m+=ndatas*4;
							mode=4;
							}
						else
						if(ret<0) {data_cheats[n].title=NULL;data_cheats[n].len_values=0;force_exit=1;}
						break;
					
					}
				}
			 else {
				  if(mode==4) n++; else data_cheats[n].title=NULL;n++;
				  break;
				  }
			
			if(force_exit) break;
			}
		fclose(fp);

		for(n=0;n<MAX_LIST_CHEATS;n++)
			if(data_cheats[n].title!=NULL) num_list_cheats=n+1; else break;
		
		}
	else
		{
		ret = fread(buff_cheats, 1, len_cheats, fp);
		fclose(fp);

		if(ret != len_cheats){
			len_cheats=0;
			free(buff_cheats);
			return 0;
			}
		}

return 1;
}


void select_file(int flag, struct discHdr *header)
{
int n,m,signal2=2,f=0,len,max_entry;
static  int posfile=0;
static int old_flag=-1;
int flash=0;
int signal=1;
int frames2=0;

u8 *mem_png=NULL;
int len_png=0;
void * texture_png=NULL;
int mode=0;


read_list_file((void *) path_file, flag);

if(flag!=old_flag)
	{
	posfile=0;
	old_flag=flag;
	}

while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	//int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	SetTexture(&text_background);


	ConfigureForTexture(10);
	GX_Begin(GX_QUADS,  GX_VTXFMT0, 4);
	AddTextureVertex(0, 0, 999, 0xfff0f0f0, 0, (frames2 & 1023));
	AddTextureVertex(SCR_WIDTH, 0, 999, 0xffa0a0a0, 1023, (frames2 & 1023)); 
	AddTextureVertex(SCR_WIDTH, SCR_HEIGHT, 999, 0xfff0f0f0, 1023, 1024+(frames2 & 1023)); 
	AddTextureVertex(0, SCR_HEIGHT, 999, 0xffa0a0a0, 0, 1024+(frames2 & 1023)); 
	GX_End();

	
   /* DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

*/
	SetTexture(NULL);
	DrawRoundFillBox(8,40, SCR_WIDTH-16, 336+8-32, 0, 0x6fffff00);			
	DrawRoundBox(8, 40, SCR_WIDTH-16, 336+8-32, 0, 2, 0xff30ffff);

	if(header)
		{
		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=ylev-32; color= 0xff000000; 
				
		bkcolor=0;
		
		autocenter=1;
		s_printf("%s",header->title);
		autocenter=0;
		}


	SetTexture(NULL);

	PX= 0; PY=ylev-32; color= 0xff000000; 
				
	bkcolor=0;
	letter_size(16,32);


	autocenter=0;

	max_entry=320/30;

	flash++;

	m=0;
	
	sizeletter=4;


	for(n=(posfile-max_entry/2)*(nfiles>=max_entry);n<posfile+max_entry;n++)
			{
			if(n<0) continue;
			if(n>=nfiles) break;
			if(m>=max_entry) break;
			
            
			PX=16;PY=48+2+m*30;
			color=0xff000000;
			
			/*if(px>=PX-8 && px<PX+8+16*38 && py>=PY-10 && py<PY+26) 
				select_game_bar=1024+n;//+2048*(kk==-1);*/
			
			if(n== posfile)
				{
				len=38;


				//if(flash & 8) 
				DrawFillBox(PX-8,PY-2-8, len*16+16, 36, 0, 0x6f00ff00);
				
				DrawBox(PX-8,PY-2-8, len*16+16, 36, 0, 2, 0xff30ffff);

				}

			if(files[n].is_directory)
				{
				if(files[n].name[0]=='.' && files[n].name[1]=='[')
					{
					color=0xffff6f00;
					s_printf("%s",&files[n].name[1]);
					}
				else
					{
					color=0xffff6f00;
				
					s_printf("<%s>",get_name_short_fromUTF8(&files[n].name[0]));
					}
				}
			else 
				{
				color=0xff000000;
	            if(files[n].is_selected) color=0xff000000;

				s_printf("%s", (char *) get_name_short_fromUTF8(&files[n].name[0]));
				}
			
			bkcolor=0;color=0xffffffff;
			m++;
			}

	if(texture_png)
		{
		SetTexture(NULL);

		DrawRoundFillBox(SCR_WIDTH/2-100-200*mode/100, 480-128-312*mode/100, 200+400*mode/100, 128+256*mode/100, 0, 0xffafafaf);
		SetTexture(&png_texture);
		DrawRoundFillBox(SCR_WIDTH/2-100-200*mode/100, 480-128-312*mode/100, 200+400*mode/100, 128+256*mode/100, 0, 0xffffffff);
		SetTexture(NULL);
		DrawRoundBox(SCR_WIDTH/2-100-200*mode/100, 480-128-312*mode/100, 200+400*mode/100, 128+256*mode/100, 0, 4, 0xff303030);

		if(mode>0) {mode+=signal2;if(mode==0) signal2=2;}
		if(mode>100) mode=100;
		
		if(mode!=0 && mode !=100) snd_fx_fx(mode);

		if(mode==100)
			{
			bkcolor=0;
			PX= 0; PY=480-40; color= 0xff000000; 
			letter_size(12,32);
			autocenter=1;
			s_printf("%s", &letrero[idioma][33][0]);
			autocenter=0;

			}
		}

	SetTexture(NULL);

	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						
						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) new_pad|=WPAD_BUTTON_A;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) old_pad|=WPAD_BUTTON_A;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) new_pad|=WPAD_BUTTON_1;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) old_pad|=WPAD_BUTTON_1;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) new_pad|=WPAD_BUTTON_MINUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) old_pad|=WPAD_BUTTON_MINUS;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) new_pad|=WPAD_BUTTON_PLUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) old_pad|=WPAD_BUTTON_PLUS;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_UP) new_pad|=WPAD_BUTTON_UP;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_UP) old_pad|=WPAD_BUTTON_UP;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_DOWN) new_pad|=WPAD_BUTTON_DOWN;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_DOWN) old_pad|=WPAD_BUTTON_DOWN;

						}
					 else
					   {px=-100;py=-100;}

				
				if(mode==0 || mode==100)
				{

				if(new_pad & WPAD_BUTTON_B)
					{
					if(mode) signal2=-2;
					else break;
					}
				}


				if(nfiles>0) if(new_pad & WPAD_BUTTON_A) 
				{
				
					if(!files[posfile].is_directory) 
						{
						
						if(is_ext(files[posfile].name,".png") && (mode==0 || mode==100))
							{
							if(texture_png!=NULL && mem_png!=NULL)
								{
								
								if(mode==0) mode=1;
								else
									{
									u8 discid[7];

									memcpy(discid,header->id,6); discid[6]=0;

									memset(temp_data,0,256*1024);
									WBFS_GetProfileDatas(discid, temp_data);
									temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=((len_png+8+1023)/1024)-1;
									
									memcpy(&temp_data[8], mem_png, len_png);

									WBFS_SetProfileDatas(discid, temp_data);

									Screen_flip();

									if(mem_png!=NULL) free(mem_png);mem_png=NULL;
									if(texture_png!=NULL) free(texture_png);texture_png=NULL;
									break;

									}
								}
							else mode=0;
								
							}
						}
							
					  else if(mode==0)
						{
						  signal=1;
		                snd_fx_yes();
						 // cambio de device
						 if(files[posfile].name[0]=='.' && files[posfile].name[1]=='[')
							{
							
							if(files[posfile].name[2]=='U') 
								{
								
								current_device=1;
								sprintf(path_file, "usb:/");
								read_list_file(NULL, flag);
								posfile=0;
							
								}
							else
							if(files[posfile].name[2]=='S') 
								{

								current_device=0;
							    sprintf(path_file, "sd:/");
								read_list_file(NULL, flag);
								posfile=0;
								
								}
							}
						 else
							{read_list_file((void *) &files[posfile].name[0], flag);posfile=0;}
						 
						 
						}
				}

			
				if(mode==0)
				{
				if(!(old_pad  & (WPAD_BUTTON_UP | WPAD_BUTTON_DOWN))) f=0;
				
				if(old_pad  & WPAD_BUTTON_UP)
					{
					if(f==0) f=2;
					else if(f & 1) f=2;
					else {f+=2;if(f>40) {f=34;new_pad|=WPAD_BUTTON_UP;}}
					}
				if(old_pad  & WPAD_BUTTON_DOWN)
					{
					if(f==0) f=1;
					else if(!(f & 1)) f=1;
					else {f+=2;if(f>41) {f=35;new_pad|=WPAD_BUTTON_DOWN;}}
					}
				if(new_pad & (WPAD_BUTTON_HOME | WPAD_BUTTON_1 | WPAD_BUTTON_2)) break;

				if((new_pad & WPAD_BUTTON_UP)) {snd_fx_tick();signal=1;posfile--;if(posfile<0) posfile=nfiles-1;}
				if((new_pad & WPAD_BUTTON_DOWN)){snd_fx_tick();signal=1;posfile++;if(posfile>=nfiles) posfile=0;} 
               
				if(signal)
				{
				FILE *fp=NULL;
				signal=0;

				if(mem_png!=NULL) free(mem_png);mem_png=NULL;
				if(texture_png!=NULL) free(texture_png);texture_png=NULL;

				if(!files[posfile].is_directory) 
					{
					
					fp=fopen(files[posfile].name,"r"); // lee el fichero de texto
					if(fp!=0)
						{
						fseek(fp,0,SEEK_END);
						len_png=ftell(fp);
						fseek(fp,0,SEEK_SET);
						if(len_png<(200*1024-8))
							{
							mem_png=malloc(len_png+128);
							if(mem_png) 
								{n=fread(mem_png,1,len_png,fp);
								if(n<0) {len_png=0;free(mem_png);mem_png=0;} else len_png=n;
								}
							} else len_png=0;
						fclose(fp);
						} else len_png=0;

					if(mem_png)
						texture_png=create_png_texture(&png_texture, mem_png, 0);
					}
				}
				} // mode==0

			}

	

	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	}

snd_fx_yes();
Screen_flip();
if(mem_png!=NULL) free(mem_png);
if(texture_png!=NULL) free(texture_png);texture_png=NULL;


}
