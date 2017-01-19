
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "smatch.h"

#include "ErrorPrint.h"
#include "ParseError.h"

#include "ParameterSelect.h"

#define MBUFFER 1024

PARSEDLLSPEC int ParameterTypeValid(char type)
{
	switch(type)
	{
		case 'a':
		case 'm':
		case 'd':
		case 'u':
		case 'x':
		case 'h':
		case 'f':
		case 't':
		case 's':
        case 'z':
			return 1;
	}
	return 0;
}


static char *ParameterPrintType(char *buffer, int max, char type)
{
	buffer[0]=0;
	switch(type)
	{
		case 'a':
		case 'm':
			SformatOutput(buffer,max-1,"%s","mac address");
			break;
		case 'd':
			SformatOutput(buffer,max-1,"%s","decimal");
			break;
		case 'u':
			SformatOutput(buffer,max-1,"%s","unsigned");
			break;
		case 'x':
		case 'h':
			SformatOutput(buffer,max-1,"%s","hexadecimal");
			break;
		case 'f':
			SformatOutput(buffer,max-1,"%s","float");
			break;
		case 't':
		case 's':
			SformatOutput(buffer,max-1,"%s","text");
			break;
        case 'z':
            SformatOutput(buffer,max-1,"%s","choice");
            break;
		default:
			break;
	}
	return buffer;
}


static char *ParameterPrint(char *buffer, int max, char type, void *ptr)
{
	char *cptr;

	buffer[0]=0;
	switch(type)
	{
		case 'a':
		case 'm':
			cptr=(char *)ptr;
			SformatOutput(buffer,max-1,"%02x:%02x:%02x:%02x:%02x:%02x",cptr[0],cptr[1],cptr[2],cptr[3],cptr[4],cptr[5]);
			break;
		case 0:
        case 'z':
		case 'd':
			SformatOutput(buffer,max-1,"%d",*((int *)ptr));
			break;
		case 'u':
			SformatOutput(buffer,max-1,"%u",*((unsigned int *)ptr));
			break;
		case 'x':
		case 'h':
			SformatOutput(buffer,max-1,"%x",*((unsigned int *)ptr));
			break;
		case 'f':
			SformatOutput(buffer,max-1,"%lg",*((double *)ptr));
			break;
		case 't':
		case 's':
			SformatOutput(buffer,max-1,"%s",(char *)ptr);
			break;
		default:
			break;
	}
	return buffer;
}


static char *ParameterPrintByValue(char *buffer, int max, char type, int value)
{
	buffer[0]=0;
	switch(type)
	{
		case 'a':
		case 'm':
		case 0:
        case 'z':
		case 'd':
		case 't':
		case 's':
		case 'f':
		default:
			SformatOutput(buffer,max-1,"%d",value);
			break;
		case 'u':
			SformatOutput(buffer,max-1,"%u",value);
			break;
		case 'x':
		case 'h':
			SformatOutput(buffer,max-1,"%x",value);
			break;
	}
	return buffer;
}


PARSEDLLSPEC void ParameterHelpLine(struct _ParameterList *cl, char *buffer, int max)
{
	int lc,nc;
	char pbuffer[MBUFFER];

	lc=0;
    if(cl->type!=0)
    {
        nc=SformatOutput(&buffer[lc],max-lc-1,"type=%s; ",ParameterPrintType(pbuffer,MBUFFER,cl->type));
        if(nc>=0)
        {
	        lc+=nc;
        }

        if(cl->minimum!=0)
        {
	        nc=SformatOutput(&buffer[lc],max-lc-1,"minimum=%s; ",ParameterPrint(pbuffer,MBUFFER,cl->type,cl->minimum));
	        if(nc>=0)
	        {
		        lc+=nc;
	        }
        }

        if(cl->maximum!=0)
        {
	        nc=SformatOutput(&buffer[lc],max-lc-1,"maximum=%s; ",ParameterPrint(pbuffer,MBUFFER,cl->type,cl->maximum));
	        if(nc>=0)
	        {
		        lc+=nc;
	        }
        }

        if(cl->def!=0)
        {
	        nc=SformatOutput(&buffer[lc],max-lc-1,"default=%s; ",ParameterPrint(pbuffer,MBUFFER,cl->type,cl->def));
	        if(nc>=0)
	        {
		        lc+=nc;
	        }
        }

        if(cl->units!=0)
        {
	        nc=SformatOutput(&buffer[lc],max-lc-1,"units=%s; ",cl->units);
	        if(nc>=0)
	        {
		        lc+=nc;
	        }
        }

        //
        // add dimension
        //
        if(cl->nx>1 || cl->ny>1 || cl->nz>1)
        {
	        nc=SformatOutput(&buffer[lc],max-lc-1,"dimension=");
	        if(nc>=0)
	        {
		        lc+=nc;
	        }
        }
        if(cl->nx>1)
        {
	        nc=SformatOutput(&buffer[lc],max-lc-1,"[%d]",cl->nx);
	        if(nc>=0)
	        {
		        lc+=nc;
	        }
        }
        if(cl->ny>1)
        {
	        nc=SformatOutput(&buffer[lc],max-lc-1,"[%d]",cl->ny);
	        if(nc>=0)
	        {
		        lc+=nc;
	        }
        }
        if(cl->nz>1)
        {
	        nc=SformatOutput(&buffer[lc],max-lc-1,"[%d]",cl->nz);
	        if(nc>=0)
	        {
		        lc+=nc;
	        }
        }
        //
        // add dimension
        //
        if(cl->nx>1 || cl->ny>1 || cl->nz>1)
        {
	        nc=SformatOutput(&buffer[lc],max-lc-1,"; ");
	        if(nc>=0)
	        {
		        lc+=nc;
	        }
        }
	}
}


//
// prints the description of the specified parameter from the structure.
//
PARSEDLLSPEC void ParameterHelpSingle(struct _ParameterList *cl, void (*print)(char *buffer))
{
	int it, is;
	int lc, nc;
	char buffer[MBUFFER],pbuffer[MBUFFER];

    if(print!=0)
    {
	    lc=0;
	    if(cl->word[0]!=0)
	    {
		    nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%s",cl->word[0]);
		    if(nc>=0)
		    {
			    lc+=nc;
		    }
	    }

	    for(it=1; it<MPWORD; it++)
	    {
		    if(cl->word[it]!=0)
		    {
			    nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,", %s",cl->word[it]);
			    if(nc>=0)
			    {
				    lc+=nc;
			    }
		    }
	    }
	    nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,": ");
	    if(nc>=0)
	    {
		    lc+=nc;
	    }
	    //
	    // add data type
	    //
	    if(cl->type!=0)
        {
	        {
		        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"type=%s; ",ParameterPrintType(pbuffer,MBUFFER,cl->type));
		        if(nc>=0)
		        {
			        lc+=nc;
		        }
	        }
	        //
	        // add special values
	        //
	        if(cl->nspecial>0)
	        {
		        if(cl->special[0].word!=0)
		        {
			        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%s",cl->special[0].word[0]);
			        if(nc>=0)
			        {
				        lc+=nc;
			        }
			        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"[%s]",ParameterPrintByValue(pbuffer,MBUFFER,cl->type,cl->special[0].code));
			        if(nc>=0)
			        {
				        lc+=nc;
			        }
		        }
		        for(is=1; is<cl->nspecial; is++)
		        {
			        if(cl->special[is].word!=0)
			        {
				        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,", %s",cl->special[is].word[0]);
				        if(nc>=0)
				        {
					        lc+=nc;
				        }
				        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"[%s]",ParameterPrintByValue(pbuffer,MBUFFER,cl->type,cl->special[is].code));
				        if(nc>=0)
				        {
					        lc+=nc;
				        }
			        }
		        }
		        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"; ");
		        if(nc>=0)
		        {
			        lc+=nc;
		        }
	        }

	        if(cl->minimum!=0)
	        {
		        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"minimum=%s; ",ParameterPrint(pbuffer,MBUFFER,cl->type,cl->minimum));
		        if(nc>=0)
		        {
			        lc+=nc;
		        }
	        }

	        if(cl->maximum!=0)
	        {
		        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"maximum=%s; ",ParameterPrint(pbuffer,MBUFFER,cl->type,cl->maximum));
		        if(nc>=0)
		        {
			        lc+=nc;
		        }
	        }

	        if(cl->def!=0)
	        {
		        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"default=%s; ",ParameterPrint(pbuffer,MBUFFER,cl->type,cl->def));
		        if(nc>=0)
		        {
			        lc+=nc;
		        }
	        }

	        if(cl->units!=0)
	        {
		        nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"units=%s; ",cl->units);
		        if(nc>=0)
		        {
			        lc+=nc;
		        }
	        }

			//
			// add dimension
			//
			if(cl->nx>1 || cl->ny>1 || cl->nz>1)
			{
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"dimension=");
				if(nc>=0)
				{
					lc+=nc;
				}
			}
			if(cl->nx>1)
			{
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"[%d]",cl->nx);
				if(nc>=0)
				{
					lc+=nc;
				}
			}
			if(cl->ny>1)
			{
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"[%d]",cl->ny);
				if(nc>=0)
				{
					lc+=nc;
				}
			}
			if(cl->nz>1)
			{
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"[%d]",cl->nz);
				if(nc>=0)
				{
					lc+=nc;
				}
			}
			//
			// add dimension
			//
			if(cl->nx>1 || cl->ny>1 || cl->nz>1)
			{
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"; ");
				if(nc>=0)
				{
					lc+=nc;
				}
			}
        }

	    if(cl->help!=0)
	    {
		    nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%s",cl->help);
		    if(nc>=0)
		    {
			    lc+=nc;
		    }
	    }

	    if(lc>0)
	    {
		    (*print)(buffer);
	    }
    }
}


//
// prints the list of parameter names from the structure.
// the function (*print)() is called once for each command.
//
PARSEDLLSPEC void ParameterHelp(struct _ParameterList *cl, int nl, void (*print)(char *buffer))
{
    int ip;

	if(print!=0)
	{
		for(ip=0; ip<nl; ip++)
		{
            ParameterHelpSingle(&cl[ip],print);
        }
	}
}

#define MAX(a,b) ((a)>=(b)?(a):(b))

//
// returns the maximum number of parameter values
//
PARSEDLLSPEC int ParameterValueMaximum(struct _ParameterList *cl)
{
	return MAX(1,cl->nx)*MAX(1,cl->ny)*MAX(1,cl->nz);
}


//
// parameters can look like name.name.name[it,jt,kt]. 
// any number of names delimited by periods. we allow partial match on each piece.
// up to three array indexes between brackets. array indexes are checked against parameter limits.
//
//
PARSEDLLSPEC int ParameterSelectIndexArray(char *buffer, struct _ParameterList *cl, int nl, int *xindex, int *yindex, int *zindex)
{
    int il, it, ic, ip;
    int good;
    int bl;
    int start;
	int exact;
	int reject;
	int ngot;
	int xi,yi,zi;
	char extra[10];
    //
	// start with no match
	//
    good= -1;
	exact=0;
	reject=0;
    //
	// array indexes start at 0
	//
	*xindex= -1;
	*yindex= -1;
	*zindex= -1;

    bl=Slength(buffer);
    if(bl>0)
    {
		//
		// find beginning of input word, skip over spaces
		//
        for(start=0; start<bl; start++)
        {
            if(buffer[start]!=' ')
            {
                break;
            }
        }
		//
		// check each parameter name
		//
        for(il=0; il<nl; il++)
        {
			for(ic=0; ic<MPWORD; ic++)
			{
				if(cl[il].word[ic]!=0)
				{
                    for(it=0, ip=0; it+start<=bl; it++, ip++)
					{
						//
						// ran out of input. still matches
						//
                        if(buffer[it+start]==' ' || buffer[it+start]==0 || buffer[it+start]=='[')
						{
                            if(cl[il].word[ic][ip]==' ' || cl[il].word[ic][ip]==0)
							{
								//
								// exact match
								//
							    //
							    // previous exact match. reject both
							    //
                                if(good>=0 && good!=il && exact)
								{
									return -1;
								}
							    //
							    // first exact match. 
							    //
                                else
								{
                                    good=il;
								    exact=1;
									reject=0;
								}
							}
							else
							{
								//
								// partial match
								//
							    //
							    // previous match. 
							    //
                                if(good>=0 && good <= il)
								{
							        //
								    // previous also partial, so both are rejected
								    //
								    if(!exact)
									{
										reject=1;
										//
										// keep going to see if we find an exact match
										//
									}
								    //
								    // previous was exact. skip this partial
								    //
								}
							    //
							    // this is the best so far. but not exact.
							    //
                                else
								{
                                    good=il;
								    exact=0;
									// to prevent data rate 0x4 (12Mbps, mcs2), search in _ParameterList rates[]
									// matches with {code_48, "48"}
									if (Scompare(cl[il].word[0], "48")==0 && buffer[0]=='4')
										reject=1;
								}
							}
                            break;
						}
						//
						// we allow partial matches on the sections of the name delimited by periods.
						// so skip ahead to next section of parameter name.
						//
                        if(buffer[it+start]=='.')
						{
							for( ; cl[il].word[ic][ip]!=0; ip++)
							{
								if(cl[il].word[ic][ip]=='.')
								{
									break;
								}
							}
							//
							// if we don't find a period, then no match
							//
							if(cl[il].word[ic][ip]!='.')
							{
								break;
							}
						}
						//
						// ran out of parameter name. but still have input, no match
						//
                        if(cl[il].word[ic][ip]==' ' || cl[il].word[ic][ip]==0)
						{
                            break;
						}
						//
						// doesn't match
						//
                        if(toupper(cl[il].word[ic][ip])!=toupper(buffer[it+start]))
						{
                            break;
						}
					}
				}
			}
		}
    }
    // 
	// found 1 good match, return it.
	//
    if(good>=0 && !reject)
    {
		//
		// look for array indexes
		//
		xi= -1;
		yi= -1;
		zi= -1;
		for(start=0; start<bl; start++)
		{
			if(buffer[start]=='[')
			{
				ngot=SformatInput(&buffer[start],"[ %d , %d , %d ] %c",&xi,&yi,&zi,extra);
				if(ngot!=3)
				{
					ngot=SformatInput(&buffer[start],"[ %d ] [ %d ] [ %d ] %c",&xi,&yi,&zi,extra);
					if(ngot!=3)
					{
						ngot=SformatInput(&buffer[start],"[ %d , %d ] %c",&xi,&yi,extra);
						if(ngot!=2)
						{
							ngot=SformatInput(&buffer[start],"[ %d ] [ %d ] %c",&xi,&yi,extra);
							if(ngot!=2)
							{
								ngot=SformatInput(&buffer[start],"[ %d ] %c",&xi,extra);
								if(ngot!=1)
								{
									ErrorPrint(ParseBadArrayIndex,&buffer[start]);
									return -1;
								}
							}
						}
					}
				}
				//
				// now we have to check that the array indexes are valid for the selected parameter
				//
				if(ngot==1)
				{
					if(xi<0 || xi>=cl[good].nx)
					{
						ErrorPrint(ParseArrayIndexBound,xi,cl[good].nx);
					}
				}
				else if(ngot==2)
				{
					if(xi<0 || xi>=cl[good].nx || yi<0 || yi>=cl[good].ny)
					{
						ErrorPrint(ParseArrayIndexBound2,xi,yi,cl[good].nx,cl[good].ny);
					}
				}
				else
				{
					if(xi<0 || xi>=cl[good].nx || yi<0 || yi>=cl[good].ny || zi<0 || zi>=cl[good].nz)
					{
						ErrorPrint(ParseArrayIndexBound3,xi,yi,zi,cl[good].nx,cl[good].ny,cl[good].nz);
					}
					//return -1;
				}
				if(ngot<3)
				{
					zi= -1;
				}
				if(ngot<2)
				{
					yi= -1;
				}
				if(ngot<1)
				{
					xi= -1;
				}
				break;
			}
		}
		*xindex=xi;
		*yindex=yi;
		*zindex=zi;
		return good;
    }
    return -1;        
}


PARSEDLLSPEC int ParameterSelectIndex(char *buffer, struct _ParameterList *cl, int nl)
{
	int it, jt, kt;

	return ParameterSelectIndexArray(buffer, cl, nl, &it, &jt, &kt);
}


PARSEDLLSPEC int ParameterSelect(char *buffer, struct _ParameterList *cl, int nl)
{
    int index;

    index=ParameterSelectIndex(buffer,cl,nl);
    if(index>=0)
    {
        return cl[index].code;
    }
    return -1000;
}

PARSEDLLSPEC char *ParameterName(int code, struct _ParameterList *cl, int nl)
{
    int il;
	//
	// check each parameter item
	//
    for(il=0; il<nl; il++)
    {
		if(cl[il].code==code)
		{
			return cl[il].word[0];
		}
	}
    return "unknown";        
}


