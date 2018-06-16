
#include "common/macros.h"

#include "fatfs_diskio_sd.hpp"
#include "third_party/FatFs/diskio.h"

#include "drv/sd/sd.hpp"

using namespace ul;
using namespace drv;

static DSTATUS status(void *ctx);
static DSTATUS initialize(void *ctx);
static DRESULT read(void *ctx, BYTE *buff, DWORD sector, UINT count);
static DRESULT write(void *ctx, const BYTE *buff, DWORD sector, UINT count);
static DRESULT ioctl(void *ctx, BYTE cmd, void *buff);

fatfs_diskio_hndlrs_t &ul::fatfs_diskio_get()
{
	static fatfs_diskio_hndlrs_t fatfs_diskio_sd =
	{
		status, initialize, read, write, ioctl
	};
	
	return fatfs_diskio_sd;
}

static DSTATUS status(void *ctx)
{
	sd *_sd = (sd *)ctx;
	sd_csd_t csd;
	
	switch(_sd->read_csd(&csd))
	{
		case SD_ERR_NONE: return RES_OK;
		case SD_ERR_LOCKED: return RES_WRPRT;
		case SD_ERR_PARAM: return RES_PARERR;
		case SD_ERR_NO_RESPONSE: return RES_NOTRDY;
		default: return RES_ERROR;
	}
}

static DSTATUS initialize(void *ctx)
{
	sd *_sd = (sd *)ctx;
	
	switch(_sd->init())
	{
		case SD_ERR_NONE: return RES_OK;
		case SD_ERR_LOCKED: return RES_WRPRT;
		case SD_ERR_PARAM: return RES_PARERR;
		case SD_ERR_NO_RESPONSE: return RES_NOTRDY;
		default: return RES_ERROR;
	}
}

static DRESULT read(void *ctx, BYTE *buff, DWORD sector, UINT count)
{
	sd *_sd = (sd *)ctx;
	int8_t res = SD_ERR_NONE;
	sd_card_t card_type = _sd->type();
	
	for(uint32_t i = 0; i < count && res == SD_ERR_NONE; i++)
	{
		if(card_type == SD_CARD_SD_V2_HI_CAPACITY)
			res = _sd->read(buff, sector + i);
		else
			res = _sd->read(buff, (sector + i) * SD_BLOCK_SIZE);
	}
	
	switch(res)
	{
		case SD_ERR_NONE: return RES_OK;
		case SD_ERR_PARAM: return RES_PARERR;
		case SD_ERR_LOCKED: return RES_WRPRT;
		case SD_ERR_NO_RESPONSE: return RES_NOTRDY;
		default: return RES_ERROR;
	}
}

static DRESULT write(void *ctx, const BYTE *buff, DWORD sector, UINT count)
{
	sd *_sd = (sd *)ctx;
	int8_t res = SD_ERR_NONE;
	sd_card_t card_type = _sd->type();
	
	for(uint32_t i = 0; i < count && res == SD_ERR_NONE; i++)
	{
		if(card_type == SD_CARD_SD_V2_HI_CAPACITY)
			res = _sd->write((BYTE *)buff, sector + i);
		else
			res = _sd->write((BYTE *)buff, (sector + i) * SD_BLOCK_SIZE);
	}
	
	switch(res)
	{
		case SD_ERR_NONE: return RES_OK;
		case SD_ERR_PARAM: return RES_PARERR;
		case SD_ERR_LOCKED: return RES_WRPRT;
		case SD_ERR_NO_RESPONSE: return RES_NOTRDY;
		default: return RES_ERROR;
	}
}

static DRESULT ioctl(void *ctx, BYTE cmd, void *buff)
{
	sd *_sd = (sd *)ctx;
	int8_t res = SD_ERR_NONE;
	DWORD start, end;
	
	switch(cmd)
	{
		case CTRL_SYNC:
			break;
		
		case GET_SECTOR_COUNT:
			*(DWORD *)buff = (DWORD)(_sd->capacity() / SD_BLOCK_SIZE);
			break;
		
		case GET_SECTOR_SIZE:
			*(WORD *)buff = (WORD)SD_BLOCK_SIZE;
			break;
		
		case GET_BLOCK_SIZE:
			*(DWORD *)buff = (DWORD)SD_BLOCK_SIZE;
			break;
		
		case CTRL_TRIM:
			// TRIM capability is enabled only when FF_USE_TRIM = 1
			
			start = ((DWORD *)buff)[0];
			//start = *(DWORD *)buff;
			end = ((DWORD *)buff)[1];
			//end = *(((DWORD *)buff) + 1);
			
			if(_sd->type() == SD_CARD_SD_V2_HI_CAPACITY)
			{
				start *= SD_BLOCK_SIZE;
				end *= SD_BLOCK_SIZE;
			}
			
			res = _sd->erase(start, end);
			break;
		
		default:
			ASSERT(0);
			res = SD_ERR_PARAM;
	}
	
	switch(res)
	{
		case SD_ERR_NONE: return RES_OK;
		case SD_ERR_PARAM: return RES_PARERR;
		case SD_ERR_LOCKED: return RES_WRPRT;
		case SD_ERR_NO_RESPONSE: return RES_NOTRDY;
		default: return RES_ERROR;
	}
}
