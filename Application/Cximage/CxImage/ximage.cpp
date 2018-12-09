// ximage.cpp : main implementation file
/* 07/08/2001 v1.00 - Davide Pizzolato - www.xdp.it
 * CxImage version 5.99c 17/Oct/2004
 */
#include "../StdAfx.h"
#include "ximage.h"

#ifndef __IPPCORE_H__
#include "ippcore.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// CxImage 
////////////////////////////////////////////////////////////////////////////////
/**
 * Initialize the internal structures
 */
void CxImage::Startup(DWORD imagetype)
{
	//init pointers
	pDib = pSelection = pAlpha = NULL;
	pLayers = NULL;
	//init structures
	memset(&head,0,sizeof(BITMAPINFOHEADER));
	memset(&info,0,sizeof(CXIMAGEINFO));
	memset(&m_ippScrInfo,0,sizeof(IPPSRCINFO));
	memset(&skp_exifinfo,0,sizeof(SKPEXIFINFO));
	//init default attributes
    info.dwType = imagetype;
	info.nQuality = 90;
	info.nAlphaMax = 255;
	info.nBkgndIndex = -1;
	info.bEnabled = true;
	SetXDPI(96);
	SetYDPI(96);

	bThumbnail = false;
	ippJPEG    = 0;
}
/**
 * Initialize the internal structures
 */
void CxImage::Startup(DWORD imagetype,const char* filename,bool thumbnail)
{
	//init pointers
	pDib = pSelection = pAlpha = NULL;
	pLayers = NULL;
	//init structures
	memset(&head,0,sizeof(BITMAPINFOHEADER));
	memset(&info,0,sizeof(CXIMAGEINFO));
	memset(&m_ippScrInfo,0,sizeof(IPPSRCINFO));
	memset(&skp_exifinfo,0,sizeof(SKPEXIFINFO));
	//init default attributes
    info.dwType = imagetype;
	info.nQuality = 90;
	info.nAlphaMax = 255;
	info.nBkgndIndex = -1;
	info.bEnabled = true;
	SetXDPI(96);
	SetYDPI(96);

	bThumbnail  = thumbnail;
	ippFileName = filename;
	bImgType    = imagetype;
	if(ippJPEG)
	{
	}
	ippJPEG     = 0;

//    TRACE1("err = %d\n", 0 );
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Empty image constructor
 * \param imagetype: (optional) set the image format, see ENUM_CXIMAGE_FORMATS
 */
CxImage::CxImage(DWORD imagetype)
{
	Startup(imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Call this function to destroy image pixels, alpha channel, selection and sub layers.
 * - Attributes are not erased, but IsValid returns false.
 *
 * \return true if everything is freed, false if the image is a Ghost
 */
bool CxImage::Destroy()
{
	//free this only if it's valid and it's not a ghost
	if (info.pGhost==NULL){
		if (pLayers) { 
			for(long n=0; n<info.nNumLayers;n++){ delete pLayers[n]; }
			free(pLayers); pLayers=0;
		}
		if (pSelection) {free(pSelection); pSelection=0;}
		if (pAlpha) {free(pAlpha); pAlpha=0;}
		if (pDib) {free(pDib); pDib=0;}
		if(0 != ippJPEG){free(ippJPEG);ippJPEG = 0;}
		if(info.pImage) info.pImage = NULL;
		ippFileName = NULL;
//		TRACE1("err = %d\n", 1 );
		return true;
	}
//	TRACE1("err = %d\n", 9 );
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Sized image constructor
 * \param dwWidth: width
 * \param dwHeight: height
 * \param wBpp: bit per pixel, can be 1, 4, 8, 24
 * \param imagetype: (optional) set the image format, see ENUM_CXIMAGE_FORMATS
 */
CxImage::CxImage(DWORD dwWidth, DWORD dwHeight, DWORD wBpp, DWORD imagetype)
{
	Startup(imagetype);
	Create(dwWidth,dwHeight,wBpp,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * image constructor from existing source
 * \param src: source image.
 * \param copypixels: copy the pixels from the source image into the new image.
 * \param copyselection: copy the selection from source
 * \param copyalpha: copy the alpha channel from source
 * \sa Copy
 */
CxImage::CxImage(const CxImage &src, bool copypixels, bool copyselection, bool copyalpha)
{
	Startup(src.GetType());
	Copy(src,copypixels,copyselection,copyalpha);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Copies the image from an exsisting source
 * \param src: source image.
 * \param copypixels: copy the pixels from the source image into the new image.
 * \param copyselection: copy the selection from source
 * \param copyalpha: copy the alpha channel from source
 */
void CxImage::Copy(const CxImage &src, bool copypixels, bool copyselection, bool copyalpha)
{
	//copy the attributes
	memcpy(&info,&src.info,sizeof(CXIMAGEINFO));
	//rebuild the image
	Create(src.GetWidth(),src.GetHeight(),src.GetBpp(),src.GetType());
	//copy the pixels and the palette, or at least copy the palette only.
	if (copypixels && pDib && src.pDib) memcpy(pDib,src.pDib,GetSize());
	else SetPalette(src.GetPalette());
	long nSize = head.biWidth * head.biHeight;
	//copy the selection
	if (copyselection && src.pSelection){
		if (pSelection) free(pSelection);
		pSelection = (BYTE*)malloc(nSize);
		memcpy(pSelection,src.pSelection,nSize);
	}
	//copy the alpha channel
	if (copyalpha && src.pAlpha){
		if (pAlpha) free(pAlpha);
		pAlpha = (BYTE*)malloc(nSize);
		memcpy(pAlpha,src.pAlpha,nSize);
	}
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Copies the image attributes from an existing image.
 * - Works only on an empty image, and the image will be still empty.
 * - <b> Use it before Create() </b>
 */
void CxImage::CopyInfo(const CxImage &src)
{
	if (pDib==NULL) memcpy(&info,&src.info,sizeof(CXIMAGEINFO));
	this->bThumbnail = src.bThumbnail;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \sa Copy
 */
CxImage& CxImage::operator = (const CxImage& isrc)
{
	if (this != &isrc) Copy(isrc);
	return *this;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Initializes or rebuilds the image.
 * \param dwWidth: width
 * \param dwHeight: height
 * \param wBpp: bit per pixel, can be 1, 4, 8, 24
 * \param imagetype: (optional) set the image format, see ENUM_CXIMAGE_FORMATS
 * \return pointer to the internal pDib object; NULL if an error occurs.
 */
void* CxImage::Create(DWORD dwWidth, DWORD dwHeight, DWORD wBpp, DWORD imagetype)
{
	// destroy the existing image (if any)
	if (!Destroy())
		return NULL;

	// prevent further actions if width or height are not vaild <Balabasnia>
	if ((dwWidth == 0) || (dwHeight == 0)){
		strcpy(info.szLastError,"CxImage::Create : width and height must be greater than zero");
		return NULL;
	}

    // Make sure bits per pixel is valid
    if		(wBpp <= 1)	wBpp = 1;
    else if (wBpp <= 4)	wBpp = 4;
    else if (wBpp <= 8)	wBpp = 8;
    else				wBpp = 24;

	// limit memory requirements (and also a check for bad parameters)
	if (((dwWidth*dwHeight*wBpp)>>8) > CXIMAGE_MAX_MEMORY){
		strcpy(info.szLastError,"CXIMAGE_MAX_MEMORY exceeded");
		return NULL;
	}

	// set the correct bpp value
    switch (wBpp){
        case 1:
            head.biClrUsed = 2;	break;
        case 4:
            head.biClrUsed = 16; break;
        case 8:
            head.biClrUsed = 256; break;
        default:
            head.biClrUsed = 0;
    }

	//set the common image informations
    info.dwEffWidth = ((((wBpp * dwWidth) + 31) / 32) * 4);
    info.dwType = imagetype;

    // initialize BITMAPINFOHEADER
	head.biSize = sizeof(BITMAPINFOHEADER); //<ralphw>
    head.biWidth = dwWidth;		// fill in width from parameter
    head.biHeight = dwHeight;	// fill in height from parameter
    head.biPlanes = 1;			// must be 1
    head.biBitCount = (WORD)wBpp;		// from parameter
    head.biCompression = BI_RGB;    
    head.biSizeImage = info.dwEffWidth * dwHeight;
//    head.biXPelsPerMeter = 0; See SetXDPI
//    head.biYPelsPerMeter = 0; See SetYDPI
    head.biClrImportant = 0;

	pDib = malloc(GetSize()); // alloc memory block to store our bitmap
    if (!pDib){
		strcpy(info.szLastError,"CxImage::Create can't allocate memory");
		return NULL;
	}

	//clear the palette
	RGBQUAD* pal=GetPalette();
	if (pal) memset(pal,0,GetPaletteSize());
	//Destroy the existing selection
#if CXIMAGE_SUPPORT_SELECTION
	if (pSelection) SelectionDelete();
#endif //CXIMAGE_SUPPORT_SELECTION
	//Destroy the existing alpha channel
#if CXIMAGE_SUPPORT_ALPHA
	if (pAlpha) AlphaDelete();
#endif //CXIMAGE_SUPPORT_ALPHA

    // use our bitmap info structure to fill in first part of
    // our DIB with the BITMAPINFOHEADER
    BITMAPINFOHEADER*  lpbi;
	lpbi = (BITMAPINFOHEADER*)(pDib);
    *lpbi = head;

	info.pImage=GetBits();

    return pDib; //return handle to the DIB
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \return pointer to the image pixels. <b> USE CAREFULLY </b>
 */
BYTE* CxImage::GetBits(DWORD row)
{ 
	if (pDib){
		if (row) {
			if (row<(DWORD)head.biHeight){
				return ((BYTE*)pDib + *(DWORD*)pDib + GetPaletteSize() + (info.dwEffWidth * row));
			} else {
				return NULL;
			}
		} else {
			return ((BYTE*)pDib + *(DWORD*)pDib + GetPaletteSize());
		}
	}
	return NULL;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \return the size in bytes of the internal pDib object
 */
long CxImage::GetSize()
{
	return head.biSize + head.biSizeImage + GetPaletteSize();
}

/**
* \return the size in bytes of the internal pDib object
*/
long CxImage::GetSrcSize()
{
	return head.biSizeImage;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \����������ԭͼ����
 */
DWORD CxImage::GetSrcHeight()
{
	return m_ippScrInfo.Height;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \����������ԭͼ�߶�
 */
DWORD CxImage::GetSrcWidth()
{
	return m_ippScrInfo.Width;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Checks if the coordinates are inside the image
 * \return true if x and y are both inside the image
 */
bool CxImage::IsInside(long x, long y)
{
  return (0<=y && y<head.biHeight && 0<=x && x<head.biWidth);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Sets the image bits to the specified value
 * - for indexed images, the output color is set by the palette entries.
 * - for RGB images, the output color is a shade of gray.
 */
void CxImage::Clear(BYTE bval)
{
	if (pDib == 0) return;

	if (GetBpp() == 1){
		if (bval > 0) bval = 255;
	}
	if (GetBpp() == 4){
		bval = (BYTE)(17*(0x0F & bval));
	}

	memset(info.pImage,bval,head.biSizeImage);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Transfers the image from an existing source image. The source becomes empty.
 * \return true if everything is ok
 */
bool CxImage::Transfer(CxImage &from)
{
	if (!Destroy())
		return false;

	memcpy(&head,&from.head,sizeof(BITMAPINFOHEADER));
	memcpy(&info,&from.info,sizeof(CXIMAGEINFO));

	if( m_ippScrInfo.Width <= 0 )
	{
		//���ԭͼ���Ⱥ͸߶�
		if(from.m_ippScrInfo.Width > 0 && from.m_ippScrInfo.Height > 0)
		{
			m_ippScrInfo.Width  = from.m_ippScrInfo.Width;
			m_ippScrInfo.Height = from.m_ippScrInfo.Height;
		}
		else
		{
			m_ippScrInfo.Width  = from.head.biWidth;
			m_ippScrInfo.Height = from.head.biHeight;
		}
	}

	pDib = from.pDib;
	pSelection = from.pSelection;
	pAlpha = from.pAlpha;
	pLayers = from.pLayers;

	memset(&from.head,0,sizeof(BITMAPINFOHEADER));
	memset(&from.info,0,sizeof(CXIMAGEINFO));
	from.pDib = from.pSelection = from.pAlpha = NULL;
	from.pLayers = NULL;
	return true;
}
/*bool CxImage::SetExifInfo(CxImage &from)
{
	memcpy(&skp_exifinfo,&from.skp_exifinfo,sizeof(SKPEXIFINFO));
	memset(&from.skp_exifinfo,0,sizeof(BITMAPINFOHEADER));

	return true;
}*/
////////////////////////////////////////////////////////////////////////////////
/**
 * (this) points to the same pDib owned by (*from), the image remains in (*from)
 * but (this) has the access to the pixels. <b>Use carefully !!!</b>
 */
void CxImage::Ghost(CxImage *from)
{
	if (from){
		memcpy(&head,&from->head,sizeof(BITMAPINFOHEADER));
		memcpy(&info,&from->info,sizeof(CXIMAGEINFO));
		pDib = from->pDib;
		pSelection = from->pSelection;
		pAlpha = from->pAlpha;
		pLayers = from->pLayers;
		info.pGhost=from;
	}
}
////////////////////////////////////////////////////////////////////////////////
/**
 * turns a 16 or 32 bit bitfield image into a RGB image
 */
void CxImage::Bitfield2RGB(BYTE *src, WORD redmask, WORD greenmask, WORD bluemask, BYTE bpp)
{
	switch (bpp){
	case 16:
	{
		DWORD ns[3]={0,0,0};
		// compute the number of shift for each mask
		for (int i=0;i<16;i++){
			if ((redmask>>i)&0x01) ns[0]++;
			if ((greenmask>>i)&0x01) ns[1]++;
			if ((bluemask>>i)&0x01) ns[2]++;
		}
		ns[1]+=ns[0]; ns[2]+=ns[1];	ns[0]=8-ns[0]; ns[1]-=8; ns[2]-=8;
		// dword aligned width for 16 bit image
		long effwidth2=(((head.biWidth + 1) / 2) * 4);
		WORD w;
		long y2,y3,x2,x3;
		BYTE *p=info.pImage;
		// scan the buffer in reverse direction to avoid reallocations
		for (long y=head.biHeight-1; y>=0; y--){
			y2=effwidth2*y;
			y3=info.dwEffWidth*y;
			for (long x=head.biWidth-1; x>=0; x--){
				x2 = 2*x+y2;
				x3 = 3*x+y3;
				w = (WORD)(src[x2]+256*src[1+x2]);
				p[  x3]=(BYTE)((w & bluemask)<<ns[0]);
				p[1+x3]=(BYTE)((w & greenmask)>>ns[1]);
				p[2+x3]=(BYTE)((w & redmask)>>ns[2]);
			}
		}
		break;
	}
	case 32:
	{
		// dword aligned width for 32 bit image
		long effwidth4 = head.biWidth * 4;
		long y4,y3,x4,x3;
		BYTE *p=info.pImage;
		// scan the buffer in reverse direction to avoid reallocations
		for (long y=head.biHeight-1; y>=0; y--){
			y4=effwidth4*y;
			y3=info.dwEffWidth*y;
			for (long x=head.biWidth-1; x>=0; x--){
				x4 = 4*x+y4;
				x3 = 3*x+y3;
				p[  x3]=src[  x4];
				p[1+x3]=src[1+x4];
				p[2+x3]=src[2+x4];
			}
		}
	}

	}
	return;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Creates an image from a generic buffer
 * \param pArray: source memory buffer
 * \param dwWidth: image width
 * \param dwHeight: image height
 * \param dwBitsperpixel: can be 1,4,8,24,32
 * \param dwBytesperline: line alignment, in bytes, for a single row stored in pArray
 * \param bFlipImage: tune this parameter if the image is upsidedown
 * \return true if everything is ok
 */
bool CxImage::CreateFromArray(BYTE* pArray,DWORD dwWidth,DWORD dwHeight,DWORD dwBitsperpixel, DWORD dwBytesperline, bool bFlipImage)
{
	if (pArray==NULL) return false;
	if (!((dwBitsperpixel==1)||(dwBitsperpixel==4)||(dwBitsperpixel==8)||
		(dwBitsperpixel==24)||(dwBitsperpixel==32))) return false;

	if (!Create(dwWidth,dwHeight,dwBitsperpixel)) return false;

	if (dwBitsperpixel<24) SetGrayPalette();

#if CXIMAGE_SUPPORT_ALPHA
	if (dwBitsperpixel==32) AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA

	BYTE *dst,*src;

	for (DWORD y = 0; y<dwHeight; y++) {
		dst = info.pImage + (bFlipImage?(dwHeight-1-y):y) * info.dwEffWidth;
		src = pArray + y * dwBytesperline;
		if (dwBitsperpixel==32){
			for(DWORD x=0;x<dwWidth;x++){
				*dst++=src[0];
				*dst++=src[1];
				*dst++=src[2];
#if CXIMAGE_SUPPORT_ALPHA
				AlphaSet(x,y,src[3]);
#endif //CXIMAGE_SUPPORT_ALPHA
				src+=4;
			}
		} else {
			memcpy(dst,src,min(info.dwEffWidth,dwBytesperline));
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \sa CreateFromArray
 */
bool CxImage::CreateFromMatrix(BYTE** ppMatrix,DWORD dwWidth,DWORD dwHeight,DWORD dwBitsperpixel, DWORD dwBytesperline, bool bFlipImage)
{
	if (ppMatrix==NULL) return false;
	if (!((dwBitsperpixel==1)||(dwBitsperpixel==4)||(dwBitsperpixel==8)||
		(dwBitsperpixel==24)||(dwBitsperpixel==32))) return false;

	if (!Create(dwWidth,dwHeight,dwBitsperpixel)) return false;

	if (dwBitsperpixel<24) SetGrayPalette();

#if CXIMAGE_SUPPORT_ALPHA
	if (dwBitsperpixel==32) AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA

	BYTE *dst,*src;

	for (DWORD y = 0; y<dwHeight; y++) {
		dst = info.pImage + (bFlipImage?(dwHeight-1-y):y) * info.dwEffWidth;
		src = ppMatrix[y];
		if (src){
			if (dwBitsperpixel==32){
				for(DWORD x=0;x<dwWidth;x++){
					*dst++=src[0];
					*dst++=src[1];
					*dst++=src[2];
#if CXIMAGE_SUPPORT_ALPHA
					AlphaSet(x,y,src[3]);
#endif //CXIMAGE_SUPPORT_ALPHA
					src+=4;
				}
			} else {
				memcpy(dst,src,min(info.dwEffWidth,dwBytesperline));
			}
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \return lightness difference between elem1 and elem2
 */
int CxImage::CompareColors(const void *elem1, const void *elem2)
{
	RGBQUAD* c1 = (RGBQUAD*)elem1;
	RGBQUAD* c2 = (RGBQUAD*)elem2;

	int g1 = (int)RGB2GRAY(c1->rgbRed,c1->rgbGreen,c1->rgbBlue);
	int g2 = (int)RGB2GRAY(c2->rgbRed,c2->rgbGreen,c2->rgbBlue);
	
	return (g1-g2);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * simply calls "if (memblock) free(memblock);".
 * Useful when calling Encode for a memory buffer,
 * from a DLL compiled with different memory management options.
 * CxImage::FreeMemory will use the same memory environment used by Encode. 
 */
void CxImage::FreeMemory(void* memblock)
{
	if (memblock)
		free(memblock);
}
////////////////////////////////////////////////////////////////////////////////
//EOF