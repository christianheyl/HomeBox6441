
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>


#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"


#include "StructPrint.h"


#define MBLOCK 10

#define MPRINTBUFFER 1024




static unsigned int Mask(int low, int high)
{
	unsigned int mask;
	int ib;
	int swap;

	if(low>high)
	{
		swap=low;
		low=high;
		high=swap;
	}

	mask=0;
	if(low<=0)
	{
		low=0;
	}
	if(high>=31)
	{
		high=31;
	}
	for(ib=low; ib<=high; ib++)
	{
		mask|=(1<<ib);
	}
	return mask;
}

static void StructComputeIndexMany(struct _StructPrint *list, int ix, int iy, int iz,
	int *index, int *many)
{
	//
	// Compute number of values to return. Single value, whole row, etc.
	//
	if(ix>=0)
	{
		if(iy>=0)
		{
			if(iz>=0)
			{
				*many=1;
			}
			else
			{
				iz=0;
				*many=list->nz;
			}
		}
		else
		{
			iz=0;
			iy=0;
			*many=list->nz*list->ny;
		}
	}
	else
	{
		iz=0;
		iy=0;
		ix=0;
		*many=list->nx*list->ny*list->nz;
	}
	//
	// compute index of first element
	//
	*index=iz+list->nz*(iy+list->ny*ix);
}


//
// ix, iy, iz specify the first element. -1 means all the elements. 
//    0, 0, 0 returns a single element. 
//    0,-1,-1 returns the entire first row, or ny*nz elements
//
// variable type of the argument value:
//    d - int *
//    p - double *
//    2,5,u,c,x - unsigned int *
//    m,t - char *
//
int StructGet(struct _StructPrint *list,
	unsigned char *mptr, int msize, 
	unsigned int *value, int max, int ix, int iy, int iz)
{
	unsigned char *mc;
	unsigned short *ms;
	unsigned int *ml;

	unsigned int vuse;
	unsigned int mask;
	int shift;

	int it;
	int iuse;

	double *fvalue;
	int fmax;
	int *ivalue;
	unsigned char *cvalue;
	int cmax;
	int index;
	int many;
	//
	// adjust return count. may truncate if not enough space.
	//
    StructComputeIndexMany(list, ix, iy, iz, &index, &many);
	if(many>max)
	{
		many=max;
	}

	ivalue=(int *)value;
	fvalue=(double *)value;
	fmax=(max*sizeof(unsigned int))/sizeof(double);
	cvalue=(unsigned char *)value;
	cmax=(max*sizeof(unsigned int))/sizeof(unsigned char);

	switch(list->size)
	{
		case 1:
			if(list->type=='t')
			{
				mc=(char *)(mptr+list->offset+index);
				for(it=0; it<list->size && it<max; it++)
				{
					cvalue[it]=mc[it];
					if(cvalue[it]==0) 
					{
						break;
					}
				}
				cvalue[max-1]=0;
			}
			else
			{
				if(list->high>=0 && list->low>=0)
				{
					mask=Mask(list->low,list->high);
					if(list->low<list->high)
					{
					    shift=list->low;
					}
					else
					{
					    shift=list->high;
					}
				}
				else
				{
					mask=0xff;
					shift=0;
				}
				for(it=0; it<many; it++)
				{
					if(list->interleave>1)
					{
						mc=(unsigned char *)(mptr+list->offset+(index+it)*list->interleave);
					}
					else
					{
						mc=(unsigned char *)(mptr+list->offset+(index+it)*list->size);
					}
					vuse=((*mc)&mask)>>shift;
					switch(list->type)
					{
						case 'p':
							fvalue[it]=(double)(0.5*(list->voff+(int)(vuse&0x3f)));
							break;
						case '2':
							if(vuse>0)
							{
								value[it]=2300+(list->voff+(int)vuse);
							}
							else
							{
								value[it]=0;
							}
							break;
						case '5':
							if(vuse>0)
							{
								value[it]=4800+5*(list->voff+(int)vuse);
							}
							else
							{
								value[it]=0;
							}
							break;
						case 'd':
							iuse=(int)vuse;
							if(vuse&0x80)
							{
								iuse=((int)vuse)|0xffffff00;
							}
							else
							{
								iuse=(int)vuse;
							}
							ivalue[it]=list->voff+iuse;
							break;
						case 'u':
						case 'c':
						default:
						case 'x':
							value[it]=(list->voff+(unsigned int)vuse)&0xff;
							break;
					}
				}
			}
			break;
		case 2:
			if(list->high>=0 && list->low>=0)
			{
				mask=Mask(list->low,list->high);
				if(list->low<list->high)
				{
				    shift=list->low;
				}
				else
				{
				    shift=list->high;
				}
			}
			else
			{
				mask=0xff;
				shift=0;
			}
			for(it=0; it<many; it++)
			{
				if(list->interleave>1)
				{
					ms=(unsigned short *)(mptr+list->offset+(index+it)*list->interleave);
				}
				else
				{
					ms=(unsigned short *)(mptr+list->offset+(index+it)*list->size);
				}
				vuse=((*ms)&mask)>>shift;
				switch(list->type)
				{
		            case 'd':
						iuse=(int)vuse;
						if(vuse&0x8000)
						{
							iuse=((int)vuse)|0xffff0000;
						}
						else
						{
							iuse=(int)vuse;
						}
			            ivalue[it]=list->voff+iuse;
						break;
		            case 'u':
		            case 'c':
					default:
		            case 'x':
			            value[it]=(list->voff+(unsigned int)vuse)&0xffff;
						break;
				}
			}
			break;
		case 4:
			if(list->high>=0 && list->low>=0)
			{
				mask=Mask(list->low,list->high);
				if(list->low<list->high)
				{
				    shift=list->low;
				}
				else
				{
				    shift=list->high;
				}
			}
			else
			{
				mask=0xff;
				shift=0;
			}
			for(it=0; it<many; it++)
			{
				if(list->interleave>1)
				{
					ml=(unsigned int *)(mptr+list->offset+(index+it)*list->interleave);
				}
				else
				{
					ml=(unsigned int *)(mptr+list->offset+(index+it)*list->size);
				}
				vuse=((*ml)&mask)>>shift;
				switch(list->type)
				{
		            case 'd':
			            ivalue[it]=list->voff+vuse;
						break;
		            case 'f':
			            fvalue[it]=(double)list->voff+(double)vuse;
						break;
		            case 'u':
		            case 'c':
					default:
		            case 'x':
			            value[it]=(list->voff+vuse)&0xffffffff;
						break;
				}
			}
			break;
		default:
			mc=(char *)(mptr+list->offset);
			if(list->type=='t')
			{
				for(it=0; it<list->size && it<max; it++)
				{
					cvalue[it]=mc[it];
					if(cvalue[it]==0) 
					{
						break;
					}
				}
				cvalue[max-1]=0;
			}
			else if(list->type=='m')
			{
				cvalue[0]=mc[0]&0xff;
				cvalue[1]=mc[1]&0xff;
				cvalue[2]=mc[2]&0xff;
				cvalue[3]=mc[3]&0xff;
				cvalue[4]=mc[4]&0xff;
				cvalue[5]=mc[5]&0xff;
			}
			else
			{
				for(it=0; it<many; it++)
				{
					cvalue[it]=mc[it]&0xff;
				}
			}
			break;
    }
	return many;
}

//
// ix, iy, iz specify the first element. -1 means all the elements. 
//    0, 0, 0 sets a single element. 
//    0,-1,-1 sets the entire first row, or ny*nz elements
//
// variable type of the argument value:
//    d - int *
//    p - double *
//    2,5,u,c,x - unsigned int *
//    m,t - char *
//
int StructSet(struct _StructPrint *list,
	unsigned char *mptr, int msize, 
	unsigned int *value, int max, int ix, int iy, int iz)
{
	unsigned char *mc;
	unsigned short *ms;
	unsigned int *ml;
	int it;
	int vuse;
	unsigned int mask;
	int shift;
	double *fvalue;
	int *ivalue;
	unsigned short *svalue;
	unsigned char *cvalue;
	int index;
	int many;
	//
	// adjust return count. may truncate if not enough space.
	//
    StructComputeIndexMany(list, ix, iy, iz, &index, &many);
	if(many>max)
	{
		many=max;
	}

	fvalue=(double *)value;
	ivalue=(int *)value;
	svalue=(unsigned short *)value;
	cvalue=(unsigned char *)value;
	switch(list->size)
	{
		case 1:
			if(list->type=='t')
			{
				mc=(unsigned char *)(mptr+list->offset+index);
				for(it=0; it<list->size && it<max; it++)
				{
					mc[it]=cvalue[it];
					if(cvalue[it]==0) 
					{
						break;
					}
				}
				mc[max-1]=0;
			}
			else
			{
				if(list->high>=0 && list->low>=0)
				{
					mask=Mask(list->low,list->high);
					if(list->low<list->high)
					{
					    shift=list->low;
					}
					else
					{
					    shift=list->high;
					}
				}
				else
				{
					mask=0xff;
					shift=0;
				}
				for(it=0; it<many; it++)
				{
					if(list->interleave>1)
					{
						mc=(unsigned char *)(mptr+list->offset+(index+it)*list->interleave);
					}
					else
					{
						mc=(unsigned char *)(mptr+list->offset+(index+it)*list->size);
					}
					switch(list->type)
					{
						case 'p':
                            vuse=(int)(fvalue[it]*2.0)-list->voff;
							break;
						case '2':
							if(value[it]>0)
							{
								vuse=value[it]-2300-list->voff;
							}
							else
							{
								vuse=0;
							}
							break;
						case '5':
							if(value[it]>0)
							{
								vuse=((value[it]-4800)/5)-list->voff;
							}
							else
							{
								vuse=0;
							}
							break;
						case 'd':
							vuse=ivalue[it]-list->voff;
							if(vuse&0x80)
							{
								vuse=vuse|0xffffff00;
							}
							break;
						case 'u':
						case 'c':
						case 'x':
						default:
							vuse=value[it]-list->voff;
							break;
					}
				    *mc= ((*mc)&(~mask))|((vuse<<shift)&mask);
				}
			}
			break;
		case 2:
			if(list->high>=0 && list->low>=0)
			{
				mask=Mask(list->low,list->high);
				if(list->low<list->high)
				{
				    shift=list->low;
				}
				else
				{
				    shift=list->high;
				}
			}
			else
			{
				mask=0xffff;
				shift=0;
			}
			for(it=0; it<many; it++)
			{
				if(list->interleave>1)
				{
					ms=(unsigned short *)(mptr+list->offset+(index+it)*list->interleave);
				}
				else
				{
					ms=(unsigned short *)(mptr+list->offset+(index+it)*list->size);
				}
				switch(list->type)
				{
		            case 'd':
						vuse=ivalue[it]-list->voff;
						if(vuse&0x8000)
						{
							vuse=vuse|0xffff0000;
						}
						break;
		            case 'u':
		            case 'c':
					default:
		            case 'x':
			            vuse=value[it]-list->voff;
						break;
				}
				*ms= ((*ms)&(~mask))|((vuse<<shift)&mask);
			}
			break;
		case 4:
			if(list->high>=0 && list->low>=0)
			{
				mask=Mask(list->low,list->high);
				if(list->low<list->high)
				{
				    shift=list->low;
				}
				else
				{
				    shift=list->high;
				}
			}
			else
			{
				mask=0xffffffff;
				shift=0;
			}
			for(it=0; it<many; it++)
			{
				if(list->interleave>1)
				{
					ml=(unsigned int *)(mptr+list->offset+(index+it)*list->interleave);
				}
				else
				{
					ml=(unsigned int *)(mptr+list->offset+(index+it)*list->size);
				}
				vuse=ml[it]&0xffffffff;
				switch(list->type)
				{
		            case 'd':
			            vuse=ivalue[it]-list->voff;
						break;
		            case 'f':
			            vuse=(int)(fvalue[it]-list->voff);
						break;
		            case 'u':
		            case 'c':
					default:
		            case 'x':
			            vuse=value[it]-list->voff;
						break;
				}
				*ml= ((*ml)&(~mask))|((vuse<<shift)&mask);
			}
			break;
		default:
			mc=(unsigned char *)(mptr+list->offset);
			if(list->type=='t')
			{
				for(it=0; it<list->size && it<max; it++)
				{
					mc[it]=cvalue[it];
					if(cvalue[it]==0) 
					{
						break;
					}
				}
				for( ; it<list->size; it++)
				{
					mc[it]=0;
				}
			}
			else if(list->type=='m')
			{
				mc[0]=cvalue[0]&0xff;
				mc[1]=cvalue[1]&0xff;
				mc[2]=cvalue[2]&0xff;
				mc[3]=cvalue[3]&0xff;
				mc[4]=cvalue[4]&0xff;
				mc[5]=cvalue[5]&0xff;
			}
			else
			{
				for(it=0; it<many; it++)
				{
					mc[it]=value[it]&0xff;
				}
			}
			break;
    }
	return many;
}


static int StructPrintIt(unsigned char *data, int type, int size, int length, int high, int low, int voff,
	char *buffer, int max)
{
	char *vc;
	short *vs;
	int *vl;
	float *vf;
	int lc, nc;
	int it;
	char text[MPRINTBUFFER];
	int vuse;
	unsigned int mask;
	int iuse;

	lc=0;
	switch(size)
	{
		case 1:
			vc=(char *)data;
			if(type=='t')
			{
				//
				// make sure there is a null
				//
				if(length>MPRINTBUFFER)
				{
					length=MPRINTBUFFER;
				}
				strncpy(text,vc,length);
				text[length]=0;

				nc=SformatOutput(&buffer[lc],max-lc-1,"%s",text);
				if(nc>0)
				{
					lc+=nc;
				}
			}
			else
			{
				for(it=0; it<length; it++)
				{
					vuse=vc[it]&0xff;
					if(high>=0 && low>=0)
					{
						mask=Mask(low,high);
						vuse=(vuse&mask)>>low;
					}
					switch(type)
					{
						case 'p':
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%.1lf",0.5*(voff+(int)(vuse&0x3f)));
							}
							break;
						case '2':
							if(vuse>0)
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",2300+(voff+(int)vuse));
							}
							else
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",0);
							}
							break;
						case '5':
							if(vuse>0)
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",4800+5*(voff+(int)vuse));
							}
							else
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",0);
							}
							break;
						case 'd':
							iuse=(int)vuse;
							if(vuse&0x80)
							{
								iuse=((int)vuse)|0xffffff00;
							}
							else
							{
								iuse=(int)vuse;
							}
							nc=SformatOutput(&buffer[lc],max-lc-1,"%+d",voff+iuse);
							break;
						case 'u':
							nc=SformatOutput(&buffer[lc],max-lc-1,"%u",(voff+(unsigned int)vuse)&0xff);
							break;
						case 'c':
							nc=SformatOutput(&buffer[lc],max-lc-1,"%1c",(voff+(unsigned int)vuse)&0xff);
							break;
						default:
						case 'x':
							nc=SformatOutput(&buffer[lc],max-lc-1,"0x%02x",(voff+(unsigned int)vuse)&0xff);
							break;
					}
					if(nc>0)
					{
						lc+=nc;
					}
					if(it<length-1)
					{
						nc=SformatOutput(&buffer[lc],max-lc-1,",");
						if(nc>0)
						{
							lc+=nc;
						}
					}
				}
			}
			break;
		case 2:
			vs=(short *)data;
			for(it=0; it<length; it++)
			{
				vuse=vs[it]&0xffff;
				if(high>=0 && low>=0)
				{
					mask=Mask(low,high);
					vuse=(vuse&mask)>>low;
				}
				switch(type)
				{
		            case 'd':
						iuse=(int)vuse;
						if(vuse&0x8000)
						{
							iuse=((int)vuse)|0xffff0000;
						}
						else
						{
							iuse=(int)vuse;
						}
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%+d",voff+iuse);
						break;
		            case 'u':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%u",(voff+(unsigned int)vuse)&0xffff);
						break;
		            case 'c':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%2c",(voff+(unsigned int)vuse)&0xff,((voff+(unsigned int)vuse)>>8)&0xff);
						break;
					default:
		            case 'x':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"0x%04x",(voff+(unsigned int)vuse)&0xffff);
						break;
				}
				if(nc>0)
				{
					lc+=nc;
				}
				if(it<length-1)
				{
			        nc=SformatOutput(&buffer[lc],max-lc-1,",");
					if(nc>0)
					{
						lc+=nc;
					}
				}
			}
			break;
		case 4:
			vl=(int *)data;
			vf=(float *)data;
			for(it=0; it<length; it++)
			{
				vuse=vl[it]&0xffffffff;
				if(high>=0 && low>=0)
				{
					mask=Mask(low,high);
					vuse=(vuse&mask)>>low;
				}
				switch(type)
				{
		            case 'd':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%+d",voff+vuse);
						break;
		            case 'u':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%u",voff+(vuse&0xffffffff));
						break;
		            case 'f':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%g",voff+vf[it]);
						break;
		            case 'c':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%4c",(voff+vuse)&0xff,((voff+(unsigned int)vuse)>>8)&0xff,((voff+(unsigned int)vuse)>>16)&0xff,((voff+(unsigned int)vuse)>>24)&0xff);
						break;
					default:
		            case 'x':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"0x%08x",(voff+vuse)&0xffffffff);
						break;
				}
				if(nc>0)
				{
					lc+=nc;
				}
				if(it<length-1)
				{
			        nc=SformatOutput(&buffer[lc],max-lc-1,",");
					if(nc>0)
					{
						lc+=nc;
					}
				}
			}
			break;
		default:
			vc=(char *)data;
			if(type=='t')
			{
				//
				// make sure there is a null
				//
				if(size>MPRINTBUFFER)
				{
					size=MPRINTBUFFER;
				}
				strncpy(text,vc,size);
				text[size]=0;
				nc=SformatOutput(&buffer[lc],max-lc-1,"%s",text);
				if(nc>0)
				{
					lc+=nc;
				}
			}
			else if(type=='m')
			{
				nc=SformatOutput(&buffer[lc],max-lc-1,"%02x:%02x:%02x:%02x:%02x:%02x",
					vc[0]&0xff,vc[1]&0xff,vc[2]&0xff,vc[3]&0xff,vc[4]&0xff,vc[5]&0xff);
				if(nc>0)
				{
					lc+=nc;
				}
			}
			else
			{
				for(it=0; it<length; it++)
				{
					vuse=vc[it]&0xff;
					nc=SformatOutput(&buffer[lc],max-lc-1,"0x%02x",(voff+(unsigned int)vuse)&0xff);
					if(nc>0)
					{
						lc+=nc;
					}
				}
			}
			break;
    }
	return lc;
}


static void StructPrintEntry(void (*print)(char *format, ...), 
	char *name, int offset, int size, int high, int low, int voff,
	int nx, int ny, int nz, int interleave,
	char type, 
	unsigned char *mptr[], unsigned int msize, int mcount, int all)
{
	int im;
	int lc, nc;
	char buffer[MPRINTBUFFER],fullname[MPRINTBUFFER];
	int it;
	int different;
	int length;
	int ix, iy, iz;

	length=nx*ny*nz;

	for(it=0; it<length; it++)
	{
#ifdef WRONG
		iy=it%(nx*ny);
		iz=it/(nx*ny);
		ix=iy%nx;
		iy=iy/nx;
#else
		iz=it%nz;
		iy=it/nz;
		ix=iy/ny;
		iy=iy%ny;
#endif
		if(nz>1)
		{
			SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d][%d][%d]",name,ix,iy,iz);
		}
		else if(ny>1)
		{
			SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d][%d]",name,ix,iy);
		}
		else if(nx>1)
		{
			SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d]",name,ix);
		}
		else
		{
			SformatOutput(fullname,MPRINTBUFFER-1,"%s",name);
		}
		fullname[MPRINTBUFFER-1]=0;
		lc=0;
		nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|ecv|%s|%d|%d|%d|%d|%d|%d|%d|%d|%c|",
			fullname,it,offset+it*interleave,size,high,low,nx,ny,nz,type);
		if(nc>0)
		{
			lc+=nc;
		}
		//
		// put value from mptr[0]
		//
		nc=StructPrintIt((mptr[0])+offset+it*interleave*size, type, size, 1, high, low, voff, &buffer[lc], MPRINTBUFFER-lc-1);
		if(nc>0)
		{
			lc+=nc;
		}
		nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
		if(nc>0)
		{
			lc+=nc;
		}
		//
		// loop over subsequent iterations
		// add value only if different than previous value
		//
		different=0;
		for(im=1; im<mcount; im++)
		{
			if(memcmp((&mptr[im-1])+offset+it*interleave*size,(&mptr[im])+offset+it*interleave*size,size)!=0)
			{
				nc=StructPrintIt((mptr[im])+offset+it*interleave*size, type, size, 1, high, low, voff, &buffer[lc], MPRINTBUFFER-lc-1);
				if(nc>0)
				{
					lc+=nc;
				}
				different++;
			}
			else
			{
				nc=SformatOutput(&buffer[lc], MPRINTBUFFER-lc-1,".");
				if(nc>0)
				{
					lc+=nc;
				}
			}
			nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
			if(nc>0)
			{
				lc+=nc;
			}
		}
		//
		// fill in up to the maximum number of blocks
		//
		for( ; im<MBLOCK; im++)
		{
			nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
			if(nc>0)
			{
				lc+=nc;
			}
		}
		//
		// print it
		//
		if(different>0 || all)
		{
			(*print)("%s",buffer);
		}
	}
}


void StructDifferenceAnalyze(void (*print)(char *format, ...), 
    struct _StructPrint *list, int nt,
	unsigned char *mptr[], int msize, int mcount, int all)
{
	int offset;
	int im;
	int lc, nc;
	char buffer[MPRINTBUFFER];
	int it;
	int io;
    //
	// make header
	//
	lc=SformatOutput(buffer,MPRINTBUFFER-1,"|ecv|name|index|offset|size|high|low|nx|ny|nz|type|");
	//
	// fill in up to the maximum number of blocks
	//
	for(im=0; im<MBLOCK; im++)
	{
		nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"b%d|",im);
		if(nc>0)
		{
			lc+=nc;
		}
	}
	(*print)("%s",buffer);

	offset=0;
	for(it=0; it<nt; it++)
	{
		//
		// first we do any bytes that are not associated with a field name
		//
		for(io=offset; io<list[it].offset; io++)
		{
            StructPrintEntry(print, "unknown", io, 1, -1, -1, 0, 1, 1, 1, 1, 'x', mptr, msize, mcount, all);
		}
		//
		// do the field
		//
        StructPrintEntry(print,
			list[it].name, list[it].offset, list[it].size, list[it].high, list[it].low, list[it].voff,
			list[it].nx, list[it].ny, list[it].nz, list[it].interleave,
			list[it].type, 
			mptr, msize, mcount, all);
        if(list[it].interleave==1 || (it<nt-1 && list[it].interleave!=list[it+1].interleave))
		{
			offset=list[it].offset+
				(list[it].size*list[it].nx*list[it].ny*list[it].nz*list[it].interleave)-
				(list[it].interleave-1); 
		}
		else
		{
			offset=list[it].offset+list[it].size; 
		}
	}
	//
	// do any trailing bytes not associated with a field name
	//
	for(io=offset; io<msize; io++)
	{
        StructPrintEntry(print, "unknown", io, 1, -1, -1, 0, 1, 1, 1, 1, 'x', mptr, msize, mcount, all);
	}
}
