#include <freerdp/assistance.h>

#include <winpr/crt.h>
#include <winpr/print.h>
#include <winpr/platform.h>
#include <winpr/image.h>
#include <freerdp/codec/interleaved.h>
#include <freerdp/codec/planar.h>
#include <freerdp/codec/bulk.h>
#include <freerdp/codec/clear.h>
#include <freerdp/freerdp.h>
#include <freerdp/codec/zgfx.h>
#include <freerdp/log.h>
#include <winpr/bitstream.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/progressive.h>

#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>


///PROGRESSIVE CODEC
#define RFX_SUBBAND_DIFFING 0x01
#define RFX_TILE_DIFFERENCE 0x01
#define RFX_DWT_REDUCE_EXTRAPOLATE 0x01

typedef struct
{
	BYTE LL3;
	BYTE HL3;
	BYTE LH3;
	BYTE HH3;
	BYTE HL2;
	BYTE LH2;
	BYTE HH2;
	BYTE HL1;
	BYTE LH1;
	BYTE HH1;
} RFX_COMPONENT_CODEC_QUANT;

typedef struct
{
	BYTE quality;
	RFX_COMPONENT_CODEC_QUANT yQuantValues;
	RFX_COMPONENT_CODEC_QUANT cbQuantValues;
	RFX_COMPONENT_CODEC_QUANT crQuantValues;
} RFX_PROGRESSIVE_CODEC_QUANT;

typedef struct
{
	UINT16 blockType;
	UINT32 blockLen;
} PROGRESSIVE_BLOCK;

typedef struct
{
	UINT16 blockType;
	UINT32 blockLen;

	UINT32 magic;
	UINT16 version;
} PROGRESSIVE_BLOCK_SYNC;

typedef struct
{
	UINT16 blockType;
	UINT32 blockLen;

	BYTE ctxId;
	UINT16 tileSize;
	BYTE flags;
} PROGRESSIVE_BLOCK_CONTEXT;

typedef struct
{
	UINT16 blockType;
	UINT32 blockLen;

	BYTE quantIdxY;
	BYTE quantIdxCb;
	BYTE quantIdxCr;
	UINT16 xIdx;
	UINT16 yIdx;

	BYTE flags;
	BYTE quality;
	BOOL dirty;

	UINT16 yLen;
	UINT16 cbLen;
	UINT16 crLen;
	UINT16 tailLen;
	const BYTE* yData;
	const BYTE* cbData;
	const BYTE* crData;
	const BYTE* tailData;

	UINT16 ySrlLen;
	UINT16 yRawLen;
	UINT16 cbSrlLen;
	UINT16 cbRawLen;
	UINT16 crSrlLen;
	UINT16 crRawLen;
	const BYTE* ySrlData;
	const BYTE* yRawData;
	const BYTE* cbSrlData;
	const BYTE* cbRawData;
	const BYTE* crSrlData;
	const BYTE* crRawData;

	UINT32 x;
	UINT32 y;
	UINT32 width;
	UINT32 height;
	UINT32 stride;

	BYTE* data;
	BYTE* current;

	UINT16 pass;
	BYTE* sign;

	RFX_COMPONENT_CODEC_QUANT yBitPos;
	RFX_COMPONENT_CODEC_QUANT cbBitPos;
	RFX_COMPONENT_CODEC_QUANT crBitPos;
	RFX_COMPONENT_CODEC_QUANT yQuant;
	RFX_COMPONENT_CODEC_QUANT cbQuant;
	RFX_COMPONENT_CODEC_QUANT crQuant;
	RFX_COMPONENT_CODEC_QUANT yProgQuant;
	RFX_COMPONENT_CODEC_QUANT cbProgQuant;
	RFX_COMPONENT_CODEC_QUANT crProgQuant;
} RFX_PROGRESSIVE_TILE;

typedef struct
{
	UINT16 blockType;
	UINT32 blockLen;

	BYTE tileSize;
	UINT16 numRects;
	BYTE numQuant;
	BYTE numProgQuant;
	BYTE flags;
	UINT16 numTiles;
	UINT16 usedTiles;
	UINT32 tileDataSize;
	RFX_RECT rects[0x10000];
	RFX_COMPONENT_CODEC_QUANT quantVals[0x100];
	RFX_PROGRESSIVE_CODEC_QUANT quantProgVals[0x100];
	RFX_PROGRESSIVE_TILE* tiles[0x10000];
} PROGRESSIVE_BLOCK_REGION;

typedef struct
{
	UINT16 blockType;
	UINT32 blockLen;

	UINT32 frameIndex;
	UINT16 regionCount;
	PROGRESSIVE_BLOCK_REGION* regions;
} PROGRESSIVE_BLOCK_FRAME_BEGIN;

typedef struct
{
	UINT16 blockType;
	UINT32 blockLen;
} PROGRESSIVE_BLOCK_FRAME_END;

typedef struct
{
	UINT16 id;
	UINT32 width;
	UINT32 height;
	UINT32 gridWidth;
	UINT32 gridHeight;
	UINT32 gridSize;
	RFX_PROGRESSIVE_TILE** tiles;
	size_t tilesSize;
	UINT32 frameId;
	UINT32 numUpdatedTiles;
	UINT32* updatedTileIndices;
} PROGRESSIVE_SURFACE_CONTEXT;

typedef enum
{
	FLAG_WBT_SYNC = 0x01,
	FLAG_WBT_FRAME_BEGIN = 0x02,
	FLAG_WBT_FRAME_END = 0x04,
	FLAG_WBT_CONTEXT = 0x08,
	FLAG_WBT_REGION = 0x10
} WBT_STATE_FLAG;

struct S_PROGRESSIVE_CONTEXT
{
	BOOL Compressor;

	wBufferPool* bufferPool;

	UINT32 format;
	UINT32 state;

	PROGRESSIVE_BLOCK_CONTEXT context;
	PROGRESSIVE_BLOCK_REGION region;
	RFX_PROGRESSIVE_CODEC_QUANT quantProgValFull;

	wHashTable* SurfaceContexts;
	wLog* log;
	wStream* buffer;
	wStream* rects;
	RFX_CONTEXT* rfx_context;
};
//PROGRESSIVE CODEC



typedef struct s_MPPC_CONTEXT MPPC_CONTEXT;

#ifdef __cplusplus
extern "C"
{
#endif

        FREERDP_LOCAL int mppc_decompress(MPPC_CONTEXT* mppc, const BYTE* pSrcData, UINT32 SrcSize,
                                          const BYTE** ppDstData, UINT32* pDstSize, UINT32 flags);

        FREERDP_LOCAL void mppc_set_compression_level(MPPC_CONTEXT* mppc, DWORD CompressionLevel);

        FREERDP_LOCAL void mppc_context_reset(MPPC_CONTEXT* mppc, BOOL flush);

        FREERDP_LOCAL MPPC_CONTEXT* mppc_context_new(DWORD CompressionLevel, BOOL Compressor);
        FREERDP_LOCAL void mppc_context_free(MPPC_CONTEXT* mppc);

#ifdef __cplusplus
}
#endif


typedef struct s_XCRUSH_CONTEXT XCRUSH_CONTEXT;

#ifdef __cplusplus
extern "C"
{
#endif

        FREERDP_LOCAL int xcrush_decompress(XCRUSH_CONTEXT* xcrush, const BYTE* pSrcData,
                                            UINT32 SrcSize, const BYTE** ppDstData, UINT32* pDstSize,
                                            UINT32 flags);

        FREERDP_LOCAL void xcrush_context_reset(XCRUSH_CONTEXT* xcrush, BOOL flush);

        FREERDP_LOCAL XCRUSH_CONTEXT* xcrush_context_new(BOOL Compressor);
        FREERDP_LOCAL void xcrush_context_free(XCRUSH_CONTEXT* xcrush);

#ifdef __cplusplus
}
#endif

typedef struct s_NCRUSH_CONTEXT NCRUSH_CONTEXT;

#ifdef __cplusplus
extern "C"
{
#endif

        FREERDP_LOCAL int ncrush_decompress(NCRUSH_CONTEXT* ncrush, const BYTE* pSrcData,
                                            UINT32 SrcSize, const BYTE** ppDstData, UINT32* pDstSize,
                                            UINT32 flags);

        FREERDP_LOCAL void ncrush_context_reset(NCRUSH_CONTEXT* ncrush, BOOL flush);

        FREERDP_LOCAL NCRUSH_CONTEXT* ncrush_context_new(BOOL Compressor);
        FREERDP_LOCAL void ncrush_context_free(NCRUSH_CONTEXT* ncrush);

#ifdef __cplusplus
}
#endif



static BOOL test_ClearDecompressExample(UINT32 nr, UINT32 width, UINT32 height,
                                        const BYTE* pSrcData, const UINT32 SrcSize)
{
	BOOL rc = FALSE;
	int status = 0;
	BYTE* pDstData = calloc(width * height, 4);
	CLEAR_CONTEXT* clear = clear_context_new(FALSE);

	if (!clear || !pDstData)
		goto fail;

	status = clear_decompress(clear, pSrcData, SrcSize, width, height, pDstData,
	                          PIXEL_FORMAT_XRGB32, 0, 0, 0, width, height, NULL);
	//printf("clear_decompress example %" PRIu32 " status: %d\n", nr, status);
	//fflush(stdout);
	rc = (status == 0);
fail:
	clear_context_free(clear);
	free(pDstData);
	return rc;
}

int TestFreeRDPCodecClear(const uint8_t *Data, size_t Size)
{
	test_ClearDecompressExample(2, 78, 17, Data, Size);
	test_ClearDecompressExample(3, 64, 24, Data, Size);
	test_ClearDecompressExample(4, 7, 15, Data, Size);
	return 0;
}

int TestFreeRDPCodecXCrush(const uint8_t *Data, size_t Size) {
 	BYTE OutputBuffer[65536] = { 0 };
        UINT32 DstSize = sizeof(OutputBuffer);
        XCRUSH_CONTEXT* xcrush = xcrush_context_new(TRUE);
        if (!xcrush)
                return 0;
        xcrush_decompress(xcrush, Data, Size, OutputBuffer, &DstSize, 0);
        xcrush_context_free(xcrush);
	return 0;
}

static int test_ZGfxDecompressFoxSingle(const uint8_t *Data, size_t Size)
{
	int rc = -1;
	int status = 0;
	UINT32 Flags = 0;
	const BYTE* pSrcData = NULL;
	UINT32 SrcSize = 0;
	UINT32 DstSize = 0;
	BYTE* pDstData = NULL;
	ZGFX_CONTEXT* zgfx = NULL;
	UINT32 expectedSize = 0;
	zgfx = zgfx_context_new(TRUE);

	if (!zgfx)
		return -1;

	SrcSize = Size;
	pSrcData = (const BYTE*)Data;
	Flags = 0;
	expectedSize = Size;
	status = zgfx_decompress(zgfx, pSrcData, SrcSize, &pDstData, &DstSize, Flags);
	if (status < 0)
		goto fail;

	rc = 0;
fail:
	free(pDstData);
	zgfx_context_free(zgfx);
	return rc;
}


int TestFreeRDPCodecZGfx(const uint8_t *Data, size_t Size)
{
	test_ZGfxDecompressFoxSingle(Data, Size);
	return 0;
}

static BOOL test_NCrushDecompressBells(const uint8_t *Data, size_t Size)
{
	BOOL rc = FALSE;
	int status = 0;
	UINT32 Flags = 0;
	const BYTE* pSrcData = NULL;
	UINT32 SrcSize = 0;
	UINT32 DstSize = 0;
	UINT32 expectedSize = 0;
	const BYTE* pDstData = NULL;
	NCRUSH_CONTEXT* ncrush = ncrush_context_new(FALSE);

	if (!ncrush)
		return rc;

	SrcSize = Size;
	pSrcData = (const BYTE*)Data;
	Flags = PACKET_COMPRESSED | 2;
	expectedSize = Size;
	status = ncrush_decompress(ncrush, pSrcData, SrcSize, &pDstData, &DstSize, Flags);
	if (status < 0)
		goto fail;

	rc = TRUE;
fail:
	ncrush_context_free(ncrush);
	return rc;
}

int TestFreeRDPCodecNCrush(const uint8_t *Data, size_t Size)
{
	test_NCrushDecompressBells(Data,Size);
	return 0;
}


#define IMG_WIDTH 64
#define IMG_HEIGHT 64
#define FORMAT_SIZE 4
#define FORMAT PIXEL_FORMAT_XRGB32


static int TestFreeRDPCodecRemoteFX(const uint8_t *Data, size_t Size)
{
	int rc = -1;
	REGION16 region = { 0 };
	RFX_CONTEXT* context = NULL;
	BYTE* dest = NULL;
	size_t stride = FORMAT_SIZE * IMG_WIDTH;
	//unsigned short buffer[4096];

	//rfx_rlgr_decode(0, Data, Size, buffer, 4096);
	

	context = rfx_context_new(FALSE);
	if (!context)
		goto fail;

	dest = calloc(IMG_WIDTH * IMG_HEIGHT, FORMAT_SIZE);
	if (!dest)
		goto fail;

	region16_init(&region);
	if (!rfx_process_message(context, Data, Size, 0, 0, dest,
	                         FORMAT, stride, IMG_HEIGHT, &region))
		goto fail;

	region16_clear(&region);
	if (!rfx_process_message(context, Data, Size, 0, 0, dest,
	                         FORMAT, stride, IMG_HEIGHT, &region))
		goto fail;
	region16_print(&region);


	rc = 0;
fail:
	region16_uninit(&region);
	rfx_context_free(context);
	free(dest);
	return rc;
}


static int test_MppcDecompressBellsRdp5(const uint8_t *Data, size_t Size)
{
	int rc = -1;
	int status = 0;
	UINT32 Flags = 0;
	const BYTE* pSrcData = NULL;
	UINT32 SrcSize = 0;
	UINT32 DstSize = 0;
	MPPC_CONTEXT* mppc = NULL;
	UINT32 expectedSize = 0;
	const BYTE* pDstData = NULL;
	mppc = mppc_context_new(1, FALSE);

	if (!mppc)
		return -1;

	SrcSize = Size;
	pSrcData = Data;
	Flags = PACKET_AT_FRONT | PACKET_COMPRESSED | 1;
	expectedSize = Size;
	status = mppc_decompress(mppc, pSrcData, SrcSize, &pDstData, &DstSize, Flags);

	if (status < 0)
		goto fail;

	rc = 0;
fail:
	mppc_context_free(mppc);
	return rc;
}

static int test_MppcDecompressBellsRdp4(const uint8_t *Data, size_t Size)
{
	int rc = -1;
	int status = 0;
	UINT32 Flags = 0;
	const BYTE* pSrcData = NULL;
	UINT32 SrcSize = 0;
	UINT32 DstSize = 0;
	MPPC_CONTEXT* mppc = NULL;
	UINT32 expectedSize = 0;
	const BYTE* pDstData = NULL;
	mppc = mppc_context_new(0, FALSE);

	if (!mppc)
		return -1;

	SrcSize = Size;
	pSrcData = (const BYTE*) Data;
	Flags = PACKET_AT_FRONT | PACKET_COMPRESSED | 0;
	expectedSize = Size;
	status = mppc_decompress(mppc, pSrcData, SrcSize, &pDstData, &DstSize, Flags);

	if (status < 0)
		goto fail;

	rc = 0;
fail:
	mppc_context_free(mppc);
	return rc;
}


static int test_MppcDecompressBufferRdp5(const uint8_t *Data, size_t Size)
{
	int rc = -1;
	int status = 0;
	UINT32 Flags = 0;
	const BYTE* pSrcData = NULL;
	UINT32 SrcSize = 0;
	UINT32 DstSize = 0;
	MPPC_CONTEXT* mppc = NULL;
	UINT32 expectedSize = 0;
	const BYTE* pDstData = NULL;
	mppc = mppc_context_new(1, FALSE);

	if (!mppc)
		return -1;

	SrcSize = Size;
	pSrcData = (const BYTE*) Data;
	Flags = PACKET_AT_FRONT | PACKET_COMPRESSED | 1;
	expectedSize = Size;
	status = mppc_decompress(mppc, pSrcData, SrcSize, &pDstData, &DstSize, Flags);

	if (status < 0)
		goto fail;

	rc = 0;
fail:
	mppc_context_free(mppc);
	return rc;
}

static int TestFreeRDPCodecMppc(const uint8_t *Data, size_t Size)
{
	test_MppcDecompressBellsRdp5(Data, Size);
	test_MppcDecompressBellsRdp4(Data, Size);
	test_MppcDecompressBufferRdp5(Data, Size);
	return 0;
}



static BOOL progressive_decode(const uint8_t* Data, size_t Size)
{
	BOOL res = FALSE;
	int rc = 0;
	BYTE* resultData = NULL;
	BYTE* dstData = NULL;
	UINT32 dstSize = 0;
	UINT32 ColorFormat = PIXEL_FORMAT_BGRX32;
	REGION16 invalidRegion = { 0 };
	wImage* image = winpr_image_new();

	UINT32 scanline,width, height;

//	wImage* dstImage = winpr_image_new();
//	char* name = "/tmp/progressive.bmp";
//	PROGRESSIVE_CONTEXT* progressiveEnc = progressive_context_new(TRUE);
	PROGRESSIVE_CONTEXT* progressiveDec = progressive_context_new(FALSE);

	region16_init(&invalidRegion);
	if ( !progressiveDec)
		goto fail;


	//scanline 4240, height 827, width 1060
	scanline = 4240;
	width = 1060;
	height = 827;


	resultData = calloc(scanline, height);
	if (!resultData)
		goto fail;

	rc = progressive_create_surface_context(progressiveDec, 0, width, height);
	if (rc <= 0)
		goto fail;

	rc = progressive_decompress(progressiveDec, Data, Size, resultData, ColorFormat,
	                            scanline, 0, 0, &invalidRegion, 0, 0);
	if (rc < 0)
		goto fail;
	
	res = TRUE;
fail:
	region16_uninit(&invalidRegion);
	//progressive_context_free(progressiveEnc);
	progressive_context_free(progressiveDec);
//	winpr_image_free(image, TRUE);
//	winpr_image_free(dstImage, FALSE);
	free(resultData);
//	free(name);
	return res;
}

static int TestFreeRDPCodecProgressive(const uint8_t *Data, size_t Size)
{
	progressive_decode(Data, Size);
}


static BOOL i_run_encode_decode(UINT16 bpp, BITMAP_INTERLEAVED_CONTEXT* encoder,
                                     BITMAP_INTERLEAVED_CONTEXT* decoder, const uint8_t *Data, size_t Size)
{
	BOOL rc2 = FALSE;
	BOOL rc = 0;
	const UINT32 w = 64;
	const UINT32 h = 64;
	const UINT32 x = 0;
	const UINT32 y = 0;
	const UINT32 format = PIXEL_FORMAT_RGBX32;
	const UINT32 bstep = FreeRDPGetBytesPerPixel(format);
	const size_t step = (w + 13) * 4;
	const size_t SrcSize = step * h;
	const float maxDiff = 4.0f * ((bpp < 24) ? 2.0f : 1.0f);
	UINT32 DstSize = SrcSize;
	BYTE* pSrcData = calloc(1, SrcSize);
	BYTE* pDstData = calloc(1, SrcSize);
	BYTE* tmp = calloc(1, SrcSize);

	if (!pSrcData || !pDstData || !tmp)
		goto fail;

	winpr_RAND(pSrcData, SrcSize);

	if (!bitmap_interleaved_context_reset(decoder))
		goto fail;

	rc = interleaved_decompress(decoder, Data, Size, w, h, bpp, pDstData, format, step, x, y, w,
	                            h, NULL);

	if (!rc)
		goto fail;

	rc2 = TRUE;
fail:
	free(pSrcData);
	free(pDstData);
	free(tmp);
	return rc2;
}


static int TestFreeRDPCodecInterleaved(const uint8_t *Data, size_t Size)
{
	BITMAP_INTERLEAVED_CONTEXT* decoder = NULL;
	int rc = -1;
	decoder = bitmap_interleaved_context_new(FALSE);

	if ( !decoder)
		goto fail;

	i_run_encode_decode(24, NULL, decoder, Data, Size);
	i_run_encode_decode(16, NULL, decoder, Data, Size);
	i_run_encode_decode(15, NULL, decoder, Data, Size);
	rc = 0;
fail:
	bitmap_interleaved_context_free(decoder);
	return rc;
}



static BOOL RunTestPlanar(BITMAP_PLANAR_CONTEXT* planar, const BYTE* srcBitmap,
                          const UINT32 srcFormat, const UINT32 dstFormat, const UINT32 width,
                          const UINT32 height, const uint8_t *Data, size_t Size)
{
	BOOL rc = FALSE;
	UINT32 dstSize = Size;
	BYTE* compressedBitmap = Data;//freerdp_bitmap_compress_planar(planar, srcBitmap, srcFormat, width,
	                           //                             height, 0, NULL, &dstSize);
	BYTE* decompressedBitmap = (BYTE*)calloc(height, width * FreeRDPGetBytesPerPixel(dstFormat));
	rc = TRUE;

	if (!decompressedBitmap)
		goto fail;

	if (!planar_decompress(planar, compressedBitmap, dstSize, width, height, decompressedBitmap,
	                       dstFormat, 0, 0, 0, width, height, FALSE))
	{
		goto fail;
	}

	rc = TRUE;
fail:
	//free(compressedBitmap);
	free(decompressedBitmap);
	return rc;
}


static BOOL TestPlanar(const UINT32 format, const uint8_t *Data, size_t Size)
{
	BOOL rc = FALSE;
	const DWORD planarFlags = PLANAR_FORMAT_HEADER_NA | PLANAR_FORMAT_HEADER_RLE;
	BITMAP_PLANAR_CONTEXT* planar = freerdp_bitmap_planar_context_new(planarFlags, 64, 64);

	if (!planar)
		goto fail;

	RunTestPlanar(planar, NULL, PIXEL_FORMAT_RGBX32, format, 64, 64, Data, Size);

	RunTestPlanar(planar, NULL, PIXEL_FORMAT_RGB16, format, 32, 32, Data, Size);

	rc = TRUE;
fail:
	freerdp_bitmap_planar_context_free(planar);
	return rc;
}

static int TestFreeRDPCodecPlanar(const uint8_t *Data, size_t Size)
{
	TestPlanar(0,Data,Size);
	return 0;
}


int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size)
{
	if (Size < 4 ) return 0;

	srand(time(0));

	int i = rand() % 18;

	if (i < 2) TestFreeRDPCodecClear(Data, Size);
	else if (i < 4) TestFreeRDPCodecXCrush(Data, Size);
	else if (i < 6) TestFreeRDPCodecZGfx(Data, Size);
	else if (i < 8) TestFreeRDPCodecNCrush(Data, Size);
	else if (i < 10) TestFreeRDPCodecRemoteFX(Data, Size);
	else if (i < 12) TestFreeRDPCodecMppc(Data, Size);
	else if (i < 14) TestFreeRDPCodecProgressive(Data, Size);
	else if (i < 16) TestFreeRDPCodecInterleaved(Data, Size);
	else if (i < 18) TestFreeRDPCodecPlanar(Data, Size);

	return 0;
}
