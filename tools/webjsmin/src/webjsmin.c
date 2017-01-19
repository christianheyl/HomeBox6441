/*==============================================================================
//
//              (C) Arcadyan Technology. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
// Project:
//		Pansy DSL router system software
//
//------------------------------------------------------------------------------
// Description:
//
//   This application is run under linux platform used to strip out all
//   the non-necessary content to optimize the HTML/JS files.
//   The based strip function have:
//    (1) strip out a demo CGI tags. (i.e. <!--DEMO, /*DEMO,..)
//        and keep the content between Real CGI tags (i.e. /*REAL, <!--REAL)
//    (2) strip out HTML or JS comment
//        like: <!--???--> or /* ?? */
//    (3) optimize Javascript code by Jsmin way(i.e: JavaScript Minifier)
//        MUST enable CONFIG_JSMIN
//        ( see http://www.crockford.com/javascript/jsmin.html)
//------------------------------------------------------------------------------
//
// Revision Details
// ----------------
//
//  $Log: webjsmin.c,v $
//  Revision 1.00  2010/10/25 14:15:46  hugh
//
//
//==============================================================================*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define ARCADYAN_PATCH
#ifdef ARCADYAN_PATCH
// for access()
#include <unistd.h>
#include <fcntl.h>
#endif //ARCADYAN_PATCH

//==============================================================================
//                                    MACROS
//==============================================================================
//#define DEBUG
#define CONFIG_JSMIN 1
#define CONFIG_SECURITY_WEB 1
#define CONFIG_JS_PACKER 1
#define CONFIG_CSS_PACKER 1

#define FNAMESIZE		1024 //255

// we need pre-reverse the buffer for build option purpose
// please check JS_DEF_OPTION_BEGIN_TAG usage in isparseSCRIPT()
#define PRE_SHIFT_SIZE 1024

#define DOCGI_BEGIN_TAG			"<!--#"
#define DOCGI_END_TAG			"-->"

// Target IE,
// a special IE tag should not striped out
#define IECOM_BEGIN_TAG			"<!--[if"
#define IECOM_END_TAG			"<![endif]-->"
// Target everything expect IE
//  http://jackosborne.co.uk/articles/ie-conditional-tags/ ªº
#define NONIECOM_BEGIN_TAG		"<!-->"
#define NONIECOM_END_TAG		"<!--<![endif]"

#define JS_OPTION_BEGIN_TAG             "/*OPT"
#define JS_OPTION_TAIL                  "*/"
#define JS_OPTION_END_TAG               "/*END_OPT*/"
#define HTML_OPTION_BEGIN_TAG   	"<!--OPT"
#define HTML_OPTION_TAIL                "-->"
#define HTML_OPTION_END_TAG             "<!--END_OPT-->"

#define HTML_MEDA_BEGIN_TAG		"<meta"
#define HTML_MEDA_END_TAG		">"

#define JS_PACKER_BEGIN_TAG		"/*JS_PACKER*/"
#define JS_PACKER_END_TAG		"/*END_JS_PACKER*/"

// format is
//  <!--OPT#  or /*OPT#
#define OPT_DEFINE                              "#"
#define OPT_NOT_DEFINE                  "!#"

// format is
//  <!--OPT@  or /*OPT@
#define JS_DEF_OPTION_BEGIN_TAG             "/*DEF#"
#define JS_DEF_OPTION_TAIL                  "*/"

#define HTML_DEF_OPTION_BEGIN_TAG       "<!--DEF#"
#define HTML_DEF_OPTION_TAIL            "-->"
#define DEF_BUILD_DEFINE                "@"

#ifndef WEB_DEMO
    #define JS_REAL_BEGIN_TAG		"/*REAL"
    #define JS_REAL_END_TAG		"REAL*/"
    #define HTML_REAL_BEGIN_TAG		"<!--REAL"
    #define HTML_REAL_END_TAG		"REAL-->"
    #define JS_DEMO_BEGIN_TAG		"/*DEMO*/"
    #define JS_DEMO_END_TAG		"/*END_DEMO*/"
    #define HTML_DEMO_BEGIN_TAG		"<!--DEMO-->"
    #define HTML_DEMO_END_TAG		"<!--END_DEMO-->"
#else
    #define JS_REAL_BEGIN_TAG 		"/*DEMO*/"
    #define JS_REAL_END_TAG		"/*END_DEMO*/"
    #define HTML_REAL_BEGIN_TAG		"<!--DEMO-->"
    #define HTML_REAL_END_TAG		"<!--END_DEMO-->"
    #define JS_DEMO_BEGIN_TAG		"/*REAL"
    #define JS_DEMO_END_TAG		"REAL*/"
    #define HTML_DEMO_BEGIN_TAG		"<!--REAL"
    #define HTML_DEMO_END_TAG		"REAL-->"
#endif

#define OUTFILE_FORMAT                  "%s/%s"

/* if conent inside, do JSMIN standard parsing*/
#define HTML_SCRIPT_BEGIN_TAG			"<SCRIPT"
#define HTML_SCRIPT_END_TAG			"</SCRIPT>"
/* ONLY used when do Mixed HTML parse*/
#define HTML_COMMENT_BEGIN_TAG			"<!--"
#define HTML_COMMENT_END_TAG			"-->"


#define C_BLANK		0x20
//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
struct{
    char* ext;
    int mod;   /* 1: mixed JS/HTML content
                *2: pure JS only
                */
} inline_c_file[] = {
	{"stm"	,1},
	{"html"	,1},
	{"htm" 	,1},
	{"asp" 	,1},
	{"bin" 	,1},
	{"js" 	,2},
	{"cgi" 	,2},
	{"txt"	,1},
#ifdef CONFIG_CSS_PACKER
	{"css"	,3},
#else
	{"css"	,1},
#endif
	{"xml"	,1},
};
static char IN_FILE[128]; //extend from 30

/* for gloabl JS parsing or not
 * if 1 means use JSMin statandard parsing
 * if 0 means the content MIX JS and HTML, use special patch parsing rule!
 *   check inline_c_file's mod is 2 will od standard JSMIN parsing, others will not!
  */
static int   F_JS_OPTIM=0;
static int   JS_OPTIM=0;
static int   JS_READY=0; /* use to identify can apply JSMIN parsing rule, even JS_OPTIM is ON */
static int   HTML_SKIP=0;

static int   org_size=0;
static int   fake_size=0;
static int   jsmin_size=0;

static set_dbg=0;//debug

#ifdef CONFIG_JSMIN

#include <stdlib.h>
#include <stdio.h>
static int   jsmin_len=0;
static unsigned char *jsmin_s=NULL; /* the begin position of buffer*/
static unsigned char *jsmin_c=NULL; /* current parsing postion*/
static unsigned char *jsmin_e=NULL; /* after JS parse last location, begin is from jsmin_s */

static int   theA;
static int   theB;
static int   theLookahead = EOF;
static int   theLast=EOF;

/// check from -joff option
//
// jsmin_off=0 (normal)
// jsmin_off=1,
//          developer use it to keep all comments data,!! for compare purpose
//          program will check "-joff option"
static int jsmin_off=0;
static int jsmin_tag_pre=0;
// if with "-secu" a security web compress will turn on
#ifdef CONFIG_SECURITY_WEB
static int secu_web=0;
#endif //CONFIG_SECURITY_WEB
#endif


#define SIMPLE_ENCODE(a,s) { \
    unsigned char *x=(unsigned char*)(a+s-1); \
    unsigned char y; \
    while(x >= a){ \
      y=((*x&0xC0)>>6);\
      *x=((*x<<2)|y); \
      x--; \
    } \
}


//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
static int compile( char *indir, char  *infile, char *outdir);
static void write_array(FILE *dbg_fp, char *buf, int len, char *name, int num);
static int proc_file_type(char *inname, char *name );

static char *g_optfile=NULL;
static char *g_skipfile=NULL;
#ifdef CONFIG_JS_PACKER
static char *g_jspacker=NULL;
// this flag setting is a encode format, but some device like iPAD not suported
static int g_jspacker_encode=0;
#endif //CONFIG_JS_PACKER
#ifdef ARCADYAN_PATCH
static int g_keep_js_nl=0;
#endif //ARCADYAN_PATCH
static int option_match(char *optname, char *value);
static char *ShowSize(int size, char *buf);
void mem_move(char *startp, char *endp, int len);

static void usage();

#ifdef CONFIG_JSMIN
static int jsmin(unsigned char *buf, int len);
static void jsmin_putc(int chr);
static int jsmin_getc();
#endif

#ifdef CONFIG_JS_PACKER
static int jspacker(unsigned char *buf, int len);
#endif
#ifdef CONFIG_CSS_PACKER
static int cssmin(unsigned char *buf, int len);
#endif
//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================



//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	int i;
	int param=0;
	char *infile, *indir, *outdir;

	infile=NULL;
	indir=NULL;
	outdir=NULL;
	g_optfile=NULL;
	g_skipfile=NULL;
	g_jspacker=NULL;
	g_jspacker_encode=0; //default no encode

	//for (i=1; i<argc; i++)	printf("     argv[%d]=[%s](%d) \n",i, argv[i], strlen(argv[i]));
	for (i=1; i<argc; i++)	{
		//printf("     argv[%d]=[%s](%d) \n",i, argv[i], strlen(argv[i]));
		if (strlen(argv[i]) == 0) continue; // if caller append some NULL space, we need drop it
		if (strcmp(argv[i],"-o")==0 && i<argc-1)
		{
			// skip previous empty
			while((++i <argc) && (strlen(argv[i])==0));
			if ( (i<argc) && (strlen(argv[i]) > 0))
			{
				g_optfile=argv[i];
			}
			continue;
		}

		if (strcmp(argv[i],"-i")==0 && (i<argc-1))
		{
			// skip previous empty
			while((++i <argc) && (strlen(argv[i])==0));
			if ( (i<argc) && (strlen(argv[i]) > 0))
			{
				g_skipfile=argv[i];
			}
			continue;
		}
		if (strcmp(argv[i],"-joff")==0)
		{
			jsmin_off=1;
			continue;
		}
#ifdef CONFIG_SECURITY_WEB
		if (strcmp(argv[i],"-secu")==0)
		{
			secu_web=1;
			continue;
		}
#endif //CONFIG_SECURITY_WEB
#ifdef CONFIG_JS_PACKER
		if (strcmp(argv[i],"-jspack")==0 && (i<argc-1))
		{
			// skip previous empty
			while((++i <argc) && (strlen(argv[i])==0));
			if ( (i<argc) && (strlen(argv[i]) > 0))
			{
				g_jspacker=argv[i];
			}
			continue;
		}
		if (strcmp(argv[i],"-jspackencode")==0 )
		{
			g_jspacker_encode=1;
			continue;
		}
#endif //CONFIG_JS_PACKER
#ifdef ARCADYAN_PATCH
		if (strcmp(argv[i],"-jnl")==0 )
		{
			g_keep_js_nl=1;
			continue;
		}
#endif
		// format is
		//  [indir] [infile] [outdir]
		// assigned to
		//  param=0->indir,
		//  param=1->infile,
		//  param=2->outdir,
		switch(param){
		case 0:
			indir = argv[i];
			break;
		case 1:
			infile = argv[i];
			break;
		case 2:
			outdir = argv[i];
			break;
		}
		param++;
	}

	if ( (infile== NULL) || (indir== NULL) || (outdir==NULL) ) {
		usage();
	}
/*
	printf(" infile=%s\n",infile);
	printf(" indir=%s\n",indir);
	printf(" outdir=%s\n",outdir);
	printf(" g_optfile=%s\n",g_optfile);
	printf(" g_skipfile=%s\n",g_skipfile);
	printf(" jsmin_off=%d\n",jsmin_off);
	printf(" secu=%d\n",secu_web);
	printf(" jspack=%s\n",g_jspacker);
*/
	if (compile( indir, infile, outdir) < 0) {
		return -1;
	}
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
static void usage()
{
	fprintf(stderr, "usage: webjsmin indir infile outdir [-o (optfile)] [-i (skipfile)] [-joff] [-secu] [-jspack (path)] [-jspackencode]\n"
			"     1. indir:  The input absolute directory path.\n"
			"     2. infile: The file with realted directory path.\n"
			"     3. outdir: The output related directory path.\n"
			"     4. optfile: support optional tags from absolute path file.(OPTION)\n"
			"     5. -joff: for developer only, it no do any jsmin optimize task.(OPTION)\n"
			"     6. -secu:  turn on security web\n"
			"     7. -jspack: turn on JS packer option, Patch is the perl parser script\n"
			"     8. -jspackencode: turn on JS E62 encode way\n");
#ifdef ARCADYAN_PATCH
	fprintf(stderr, "     9. -jnl: keep \\n in JavaScript for debug purpose.\n");
#endif
	fprintf(stderr, "Example:\n"
	                "   ./webjsmin /tmp/html_tmp/doc global.js web_temp/doc /usr/include/proj.inc \n\n"
	                "      It will optimize the \"global.js\" file under \"html_tmp/doc\" base-on option file \n"
	                "      at \"/user/include/proj.inc\" and put the result into \"web_temp/doc\" directory\n"
	                "      name also \"global.js\" \n");
	exit(2);
}
//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------

static char *ShowSize(int _sz, char *sz_buf)
{

	char sz_u[4][10]={"","K","M" ,'\0'};
	int i=0;
	int sz=_sz;
	while(sz > 1024){
		sz=sz/1024;
		i++;
		if(i > 2) {
			i=0;
			break;
		}
	}
	sprintf(sz_buf, "%d%s", sz, sz_u[i]);
	return sz_buf;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
static int option_match(char *optname, char *value)
{
	FILE *opt_fp;
	char *line=NULL;
	char *p;
	int len=0;
	int sz=0;
	int rc=0;

	if(g_optfile){
		if ((opt_fp = fopen(g_optfile, "r")) == NULL ) {
			printf( "****Error: Can't open file: %s\n", g_optfile);
			return rc;
		}
		/* Option file is Gmodule.inc
		 * format is [xxxx]=y
		 * we need to compare the string exist or not for [xxx]=
		 * if match [xxx] and next char is "=" will mtach.
		 */
		//printf(" check option:%s\n",optname);
		while ( (sz=getline(&line, &len, opt_fp)) != -1) {
			if((p = strstr(line, optname)) == NULL)	continue;
			if( (!memcmp(line, optname, strlen(optname))) &&
			    ( line[strlen(optname)] == '=' ) )
			{
				rc=1; /* define been found*/
				if(line[strlen(optname)+1] == '0' ){
					/* if valiable is "[xxx]=0", still no defined*/
					//printf("(drop %s) ",optname);
					rc=0;
				}
				if(value)
				{
					line[sz-1]='\0'; //strip last \n
					sprintf(value, line+strlen(optname)+1);
				}
				fclose(opt_fp);
				//if *line is NULL, then getline() will allocate a buffer
				//for storing the line, which should be freed by the user program
				free(line);
				//printf("Match %s\n",optname);
				return rc;
			}
		}
		fclose(opt_fp);
		//if *line is NULL, then getline() will allocate a buffer
		//for storing the line, which should be freed by the user program
		free(line);
		return rc;
	}
	return rc; /* default no support it*/
}
// NOTE:
// thie function will over write the content before the endof optname
// by @[opt tag]@...@[opt tag]@
static unsigned char *option_value(unsigned char *last_buf, unsigned char *optstr){
	unsigned char *s=optstr;
	unsigned char *last=optstr;
	unsigned char *optname;
	unsigned char value[1024];
	unsigned char *p;
	unsigned char tmp[1024]={0};
	int sz=strlen(DEF_BUILD_DEFINE);

	if(!g_optfile) return last_buf;
	//printf("option_value(%s)..\n",optstr);
	while(*s)
	{

		// found first "@"
		if((optname = strstr(s, DEF_BUILD_DEFINE)))
		{
			// if s != optname, need store previous content
			if(s != optname)
			{
				memcpy(value, s, (int)(optname-s));
				value[(int)(optname-s)]='\0';
				strcat((unsigned char *)tmp, value);
			}
			// find next "@"
			if(!(p = strstr(optname+sz, DEF_BUILD_DEFINE)))
			{
				// no found the next "@",just return
				strcat(tmp,s);
				printf("ERROR :drop %s",s);
				break;
			}
			//found it
			*p='\0';
			s=p+1;

			// open option file for value
			value[0]='\0';
			option_match(optname+sz, (char*)&value);
			if(strlen(value) !=0)
			{
				strcat((unsigned char *)tmp, value);
				//printf("get %s",value);
			}else{
				// not match, roll back
				strcat((unsigned char *)tmp, optname);
				//printf("copy %s",optname);
				memcpy(p,DEF_BUILD_DEFINE, strlen(DEF_BUILD_DEFINE));

				s=p;
			}
		}else{
			strcat(tmp,s);
			break;
		}
	}

	sz=strlen(tmp);
	// we need over write the content before last_buf
	// calcuate the offset e.g.: last_buf-strlen(tmp)
	p=last_buf-sz;
	memcpy((void*)p, (void*) &tmp, sz);
	return p;

}
//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------

static int compile( char *indir, char *infile, char *outdir )
{
	FILE			*infp, *c_outfp, *c1_outfp, *dbg_outfp;
	char			in_file[FNAMESIZE];
	char			out_file[FNAMESIZE];
	char 			basic_name[FNAMESIZE];
	char			*cp, *sl;
	char			buf[1000];
	int			len, nsection;
	int			process_inline_c;
	char 			*proc_blk;
	int 			proc_blk_len;
	char			*last, *next;
	char			inputf[FNAMESIZE];
	int 			size;
	char			cmdstr[255];
	struct stat 	st;
	int 			rc;
	int 			bad = 1;
	int				do_cgi=0;
	char			sz_buf[3][30];
        int outfp_len;

		/* open source file to read */
		sprintf(in_file,OUTFILE_FORMAT, indir, infile);
		/* keep IN_FILE for JSMIN present only*/
		sprintf(IN_FILE,"%s",infile);
		if ((infp = fopen(in_file, "r")) == NULL ) {
			fprintf(stderr, "Can't open infile: %s\n", in_file);
			return -1;
		}
		// cut the content till last "???.xxx"
		/* Remove the prefix and add a leading "/" when we print the path */
		strcpy(inputf, infile);

		cp = (char*) &infile[0];
		while((sl = strchr(cp, '/')) != NULL) {
			cp=sl+1;
		}
		if (*cp == '/') {
			cp++;
		}
		strcpy((char*)&inputf[0], cp);
		//printf("--> infile is %s\n",inputf);

		process_inline_c = proc_file_type(inputf, basic_name);
		//printf("process_inline_c:%d\n", process_inline_c);
		#ifdef CONFIG_JSMIN
		JS_OPTIM=F_JS_OPTIM=JS_READY=(process_inline_c==2) ? 1:0;
                theA=EOF;
		#endif
		//printf("basic_name:%s\n", basic_name);
		//

#ifdef ARCADYAN_PATCH
		// Create, if folder not exist.
		// For multi-level issue, replace mkdir function call by command	.
		// Bryce Liu ++
		if(access(outdir, 0) != F_OK)
		{
			//mkdir(outdir, 0755);
			char cmd[1024] = {0};
			sprintf(cmd, "mkdir -p %s", outdir);
			system(cmd);
		}
#endif //ARCADYAN_PATCH

		sprintf(out_file, OUTFILE_FORMAT, outdir, infile); /*"%s/%s"*/
		if ((dbg_outfp = fopen(out_file, "w")) == NULL ) {
			fprintf(stderr, "Can't open out file: [%s]\n", out_file);
			return -1;
		}


		nsection = 0;
		proc_blk = 0;
		proc_blk_len = 0;
		last = 0;
		{
		       #include <sys/types.h>
		       #include <sys/stat.h>
		       #include <unistd.h>
			/*
			 * get the ui page file sizes
			 */
			if (rc=stat(in_file, &st))
			{
				printf("stat %s err =%d, %s\n", rc,in_file, strerror(rc));
				goto cmp_bad;
			}
			if (st.st_size)
			{
				len=(int)st.st_size;
				//printf("read file size=%d\n",len);
                                org_size=len;
				//len=10000; //(len+256)&~256;

				if ((proc_blk=(char*) malloc( len+PRE_SHIFT_SIZE+2 ))==0)
				{
					printf("malloc err\n");
					goto cmp_bad;
				}
				memset((void *)proc_blk, 0, (size_t)(len+PRE_SHIFT_SIZE+2));
				proc_blk+=PRE_SHIFT_SIZE; // shift to pre-keep offset

				if ( (rc=fread(proc_blk, len, 1,  infp)) == 1 ) {
					proc_blk_len =len;
    				proc_blk[proc_blk_len] = ' '; // fix me: here just for some pages which last line is empty, last character will losted
					proc_blk[proc_blk_len+1] = '\0';
				}
				else
				{
					printf("fread errr\n");
					goto cmp_bad;
				}
			}
			else
			{
				printf("file size=0\n");
				goto cmp_bad;
			}
		}
		//  process_inline_c
		// 1: for HTML
		// 2: for JS/HTML mixed format
		// 3: CSS packer
#ifdef CONFIG_CSS_PACKER
		if(process_inline_c && process_inline_c !=3) {
#else
		if(process_inline_c!=0) {
#endif
			/* *********************************************
			 * a stand html or cgi files, which exist the CGI tags inside
			 *
			 */
			int outbytes = 0;

                        fake_size=proc_blk_len;

			//printf("proc_blk=%x %d\n",proc_blk, proc_blk_len);
			//last = proc_blk;
			last = proc_blk-1; //Why? some UI first line is not \n, it cause jsmin verify incorrect, more shit one and it default is '\0'

			#ifdef CONFIG_JSMIN
			outfp_len=jsmin(last, proc_blk_len+1);
			#else
			outfp_len=proc_blk_len+1;
			#endif

			if( outfp_len){
			    	//printf("do [%s]\n",g_jspacker);
			    	#ifdef CONFIG_JS_PACKER
			    	if(g_jspacker!=NULL){
				    outfp_len=jspacker(last, outfp_len);
				}
			    	#endif //CONFIG_JS_PACKER
				write_array(dbg_outfp, last, outfp_len, basic_name, nsection);
			}
			#ifdef CONFIG_JSMIN
			jsmin_size=outbytes+outfp_len;
			printf("%s: %s -> %s (%2.2d %% jsmin)\n", IN_FILE,
								ShowSize(org_size,(char *)&sz_buf[0]),
								ShowSize(jsmin_size,(char *)&sz_buf[1]),
								((fake_size-jsmin_size)*100/fake_size));
			#else
			printf("%s: %d -> %d \n", IN_FILE,
						  ShowSize(org_size,(char *)&sz_buf[0]),
						  ShowSize(fake_size,(char *)&sz_buf[1]));
			#endif
#ifdef CONFIG_CSS_PACKER
		}else if( process_inline_c && process_inline_c ==3) {
		     // special for CSS file
		     last = proc_blk;
		     fake_size=proc_blk_len;
		     outfp_len=cssmin(last, proc_blk_len+1);
		     printf("%s: %s -> %s (%2.2d %% cssmin)\n", IN_FILE,
								ShowSize(org_size,(char *)&sz_buf[0]),
								ShowSize(outfp_len,(char *)&sz_buf[1]),
								((fake_size-outfp_len)*100/fake_size));
		     if( outfp_len){
		     	write_array(dbg_outfp, last, outfp_len, basic_name, nsection);
		     }
#endif
		}else {
			/* *********************************************
			 *  process_inline_c=0
			 *  general binary files, ex. *.jpg, *.gif,...
			 *  this kind of files may direct copy it
			 */
			//printf("open out file: [%s]\n", out_file);
			  printf("%s: %s\n",
			  		  IN_FILE,
			  		  ShowSize(proc_blk_len, (char *)&sz_buf[0]) );
			write_array(dbg_outfp, proc_blk, proc_blk_len, basic_name, nsection);
			nsection++;
		}
done:
		if(proc_blk){
			free(proc_blk-PRE_SHIFT_SIZE);
		}

		bad = 0;
cmp_bad:
		if(infp)
			fclose(infp);
		if(dbg_outfp)
			fclose(dbg_outfp);
		if(!bad)
		{
		}
		else
		{
			sprintf(out_file, OUTFILE_FORMAT, outdir, basic_name); /*"%s/%s"*/
			unlink(out_file);
		}

	return 0;

}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//		conveter file name, ex. setup.asp -> setup_asp
//
// PARAMETERS
//
//
// RETURN
//		return 	1 -> need to process inline c
//              2 -> need to process inline c, and a JS file
//  			0 -> no need to process inline c
//------------------------------------------------------------------------------
static int proc_file_type(char *inname, char* name )
{
    FILE *skip_fp;
    char *line=NULL;
    int len=0;
    char *p;

    int fl;
    int i;
    char *c;

    // we set skip file list
	if(g_skipfile){
		if ((skip_fp = fopen(g_skipfile, "r")) != NULL ) {

			/* Option file is weblist.skip
			 * format is [xxxx]\n
			 * we need to compare the string exist or not for [xxx]
			 * if match [xxx] and next char is "=" will mtach.
			 */
			while ( (i=getline(&line, &len, skip_fp)) != -1) {

				//getline() will get all line include new line char(\n), skip it to get whole string
				// add by kilin 2012.06.14
				line[strlen(line)-1] = '\0';
				//printf("!!!check Match [%s][%d] <-->[%s][%d]\n",line, strlen(line), inname, strlen(inname));
				if((p = strstr(line, inname)) == NULL)
					continue;
				//printf(" Match [%s][%d] <-->[%s][%d]\n",line, strlen(line), inname, strlen(inname));
				//====================================================
				//	case:
				//		[skip_list]		[target]		[skip?]
				//1)		abcd.htm		abc			X
				//2)		abcd.htm		bcd.htm			X
				//3)		abcd.htm		abcd.ht			X
				//4)		abcd.htm		abcd.htm		O
				//5)		doc/test/abcd.htm		abcd.htm	O
				//6)		doc/test/xabcd.html		abcd.htm	x
				//====================================================
				// NOTE: for case 5 should also skip also, the rule is previous is "/"
				//       and p not at the begin of line
				if(p != line){
					// NOTE: linux only "/", DOS is "\"
					if((*(p-1) != '/') && (*(p-1) == '\\')){
						//printf("Skip Match %s\n",inname);
						continue; // case 2,6
					}
				}

				if( (!memcmp(p, inname, strlen(inname))) && // when case 1,3,4,5
					(strlen(p) ==strlen(inname)))       // also case 1,3 happen
				{
					/* skip file been found*/
					fclose(skip_fp);
					//if *line is NULL, then getline() will allocate a buffer
					//for storing the line, which should be freed by the user program
					free(line);
					//printf("Skip Match %s\n",inname);
					return 0;
				}
			}
			fclose(skip_fp);
			//if *line is NULL, then getline() will allocate a buffer
			//for storing the line, which should be freed by the user program
			free(line);
		}else{
			// no exist skip file will skip do normal case
			//printf( "****Error: Can't open file: %s\n", g_skipfile);
		}
		//printf("No Skip Match %s\n",inname);
	}
	strcpy(name, inname);
	c = name;
	fl = strlen( name );
	for ( i = 0; i < sizeof(inline_c_file) / sizeof(*inline_c_file); ++i )
	{
		int el = strlen( inline_c_file[i].ext );
		if ( strcasecmp( &(name[fl - el]), inline_c_file[i].ext ) == 0 ){
			return inline_c_file[i].mod;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
static void write_array( FILE *dbg_fp, char *buf, int len, char *name, int num)
{
	int i, j;

	//printf("write_array: len:%d\n", len);
	if(len == 0)
		return;

#ifdef CONFIG_SECURITY_WEB
	if(secu_web==1)
		SIMPLE_ENCODE((unsigned char*)buf, len);
#endif //CONFIG_SECURITY_WEB

	for (i = 0; i < len; ) {
		//fprintf(h_fp, "\t");
		for (j = 0; (i < len)&&(j < 16); j++) {
				fprintf(dbg_fp, "%c", (char)buf[i]);
			i++;
		}
		//fprintf(h_fp, "\n");

	}
}


#ifdef CONFIG_JSMIN
/* jsmin.c
   2007-05-22

	Copyright (c) 2002 Douglas Crockford  (www.crockford.com)

	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the "Software"), to deal in
	the Software without restriction, including without limitation the rights to
	use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is furnished to do
	so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	The Software shall be used for Good, not Evil.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/
#define SUPPORT_DYNAMIC_JS 1
#define SUPPORT_REMOVE_HTML_COMMENT 1

/* isAlphanum -- return true if the character is a letter, digit, underscore,
        dollar sign, or non-ASCII character.
*/
static int
isAlphanum(int c)
{
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' ||
        c > 126);
}

static void dumpcontent(char *ptr, int sz)
{
	#ifdef DEBUG
	int i;
	printf(">>>>>>>>>>\n");
	for(i=0; i < sz;i++){

		if(*(ptr+i) < 32)
			printf("[%x]",*(ptr+i));
		else
			printf("%c",*(ptr+i));
		if((i+1)%32==0) printf("\n");
	}
	printf("\n");
	printf("<<<<<<<<<<<<\n");
	#endif
}
void showAB(void)
{
        if(theA >=32)
            printf("[%c]",theA);
        else
            printf("[%x]",theA);
        if(theB >=32)
            printf(":[%c]\n",theB);
        else
            printf(":[%x]\n",theB);
}
/* buf: the conent
 * old:
 *     F_JS_OPTIM=1
 *       a. if found <script means return 1
 *       b. if found </script means return 1
 *     F_JS_OPTIM=0
 *       a. if found <script
 *             if old =1 then ERR, return 1
 *             if old =0 then , return 1
 *       b. if found </script means return 0
 *             if old =1 then , return 0
 *             if old =0 then ,ERR return 0
 *       c. no found return old
 *  NOTE: 20091216
 *    if some error happen still keep "old" original value.
 * return :
 *     1: means use JSMIN standard parsing rule
 *     0: means mixed HTML and JS parsing rule
 */
static in_script=0;

static int isparseSCRIPT(unsigned char* buf, int old)
{
   int i;
   unsigned char *optn,*optb;
   unsigned char* p;
   unsigned char* p1; //SUPPORT_REMOVE_HTML_COMMENT
   unsigned char* p2, *pp, *p3;
   int casecade=0;
   int inverse=0;
   unsigned char temp[1024];
   #ifdef DEBUG
   if(set_dbg){
       	   printf("=======buf==================\n");
	   dumpcontent(buf,50);
	   printf("=======jsmin==================\n");
	   dumpcontent(jsmin_e-50,50);
   }
   #endif //DEBUG

   if(old ==1){
        p=(unsigned char*)buf;
	/***************************************************
	*   support option JS_DEMO_BEGIN_TAG
	***************************************************/
	if( !memcmp((void *) buf, JS_DEMO_BEGIN_TAG, strlen(JS_DEMO_BEGIN_TAG)) )
	{
	    	//printf("found /*DEMO*/ at %d\n", jsmin_c-jsmin_s);
		p+=strlen(JS_DEMO_BEGIN_TAG);
		/* check END_DEMO*/
		if((p1 = strstr(p, JS_DEMO_END_TAG)) == NULL ){
			printf("****Error: can not find %s\n", JS_DEMO_END_TAG);
			return old; /*keep old*/
		}
		// this is trick way to remove the last character, and make current point to here.
		// because jsmin will get next character
		p1+=strlen(JS_DEMO_END_TAG);
		*(p1-1)=(unsigned char) ('\n'); /* strip as new line, why ? JSmin will  drop \n */
		jsmin_c=p1-1; /* point to "\n" character*/
		//printf("Drop to %d\n",jsmin_c-jsmin_s);
		#ifdef DEBUG
		dumpcontent(p1,100);
		#endif //DEBUG

		return old;
	} //JS_DEMO_BEGIN_TAG
	/***************************************************
	 *   support option JS_REAL_BEGIN_TAG
	 ***************************************************/
	if( !memcmp((void *) buf, JS_REAL_BEGIN_TAG, strlen(JS_REAL_BEGIN_TAG)) )
	{
	    	//printf("found /*REAL at %d\n", jsmin_c-jsmin_s);
		p+=strlen(JS_REAL_BEGIN_TAG);
		/* check REAL*/
		if((p1 = strstr(p, JS_REAL_END_TAG)) == NULL ){
			printf("****Error: can not find %s\n", JS_REAL_END_TAG);
			return old; /*keep old*/
		}
		jsmin_c=p;/* set pointer after REAL */
		// move from REAL*/ to invalid /*??*/
		memcpy(p1,"/*??",4);

		#ifdef DEBUG
		dumpcontent(p1,100);
		#endif //DEBUG

		return old;
	} //JS_REAL_BEGIN_TAG
        /***************************************************
         *   support option JS_OPTION_BEGIN_TAG
         ***************************************************/
	if( !memcmp((void *) buf, JS_OPTION_BEGIN_TAG, strlen(JS_OPTION_BEGIN_TAG)) )
	{
	        //printf("found /*OPT# at %d\n", jsmin_c-jsmin_s);
		p+=strlen(JS_OPTION_BEGIN_TAG);
		optn=p;
		/* check END_OPT*/
		if((p1 = strstr(p, JS_OPTION_END_TAG)) == NULL ){
			printf("****Error: can not find %s\n", JS_OPTION_END_TAG);
			return old; /*keep old*/
		}
		// NOTE: 2013/4/25 support recursive OPTION
		// recursive look check if find others JS_OPTION_BEGIN_TAG
		casecade=0;
		pp=p;
		//dumpcontent(p1,30);
		//p1+=strlen(JS_OPTION_BEGIN_TAG);
		while(*pp)
		{
		    if((p2 = strstr(pp, JS_OPTION_BEGIN_TAG)) != NULL )
		    {
				// if any /*OPT exist, check if p2 < p1 then we need to skip it
            	if(p2 < p1)
            	{
					//printf("****find: rescursive find %s\n", JS_OPTION_BEGIN_TAG);
					pp=p2+strlen(JS_OPTION_BEGIN_TAG);
					p1+=strlen(JS_OPTION_BEGIN_TAG);
					casecade++;
					if((p1 = strstr(p1, JS_OPTION_END_TAG)) == NULL ){
						printf("****Error: can not find %s\n", JS_OPTION_END_TAG);
						return old; /* keep old*/
					}
				}else{
					break;
				}
			}else{
				break;
			}
		}
		if(!*pp){
			printf("****Error: can not find %s\n", JS_OPTION_END_TAG);
			return old; /* keep old*/
		}
		//skip thise casecade
		// no body find <!--OPT# p1 is at the END_OPT
		//printf("****after casecade [%d] find \n", casecade);
		//dumpcontent(p1,30);

		/* check OPT#, or OPT!#  */
		if(!memcmp(OPT_NOT_DEFINE, p, strlen(OPT_NOT_DEFINE)))
		{
			inverse = 1;
			optn += strlen(OPT_NOT_DEFINE);
		}
		else if (!memcmp(OPT_DEFINE, p, strlen(OPT_DEFINE)))
		{
			inverse = 0;
			optn += strlen(OPT_DEFINE);
		}
		else
		{
			printf("****Error: % has not correct format\n", JS_OPTION_BEGIN_TAG);
			return old; /*keep old*/
		}
		/* give string an end */
		if((optb = strstr(optn, JS_OPTION_TAIL)))
		{
			*optb = '\0';
			optb += strlen(JS_OPTION_TAIL);
		}
		else
		{
			printf("****Error: can not find end of option %s\n",JS_OPTION_TAIL);
			return old; /*keep old*/
		}
		//printf("%s JS OPT=%s\n", inverse?"NOT DEF": "DEF", optn);
		jsmin_c=optb;
		if(!option_match(optn,0)^inverse)
		{
			/* no mtach skip to JS_OPTION_END_TAG*/
			#ifdef DEBUG
			printf("%s no match %s\n",JS_OPTION_BEGIN_TAG, optn);
			#endif
			p1+=strlen(JS_OPTION_END_TAG);
			*(p1-1)=(unsigned char) ('\n'); /* strip as new line, why? because jsmin will skip \n */
			jsmin_c=p1-1;
			//printf("Drop to %d\n",jsmin_c-jsmin_s);
			return old; /* keep old*/
		}

		// move /*END_OPT to invalid  /*???????
		memcpy(p1,"/*???????",9);

		#ifdef DEBUG
		dumpcontent(p1,100);
		#endif //DEBUG

		return old;
	} //JS_OPTION_BEGIN_TAG
	/***************************************************
	 *   support option JS_DEF_BEGIN_TAG
	 ***************************************************/
	if( !memcmp((void *) buf, JS_DEF_OPTION_BEGIN_TAG, strlen(JS_DEF_OPTION_BEGIN_TAG)) )
	{
	    //printf("found /*DEF# at %d\n", jsmin_c-jsmin_s);
		p+=strlen(JS_DEF_OPTION_BEGIN_TAG);
		optn=p;
		//printf("get...\n");
		/* check last end */
		if((p1 = strstr(p, JS_DEF_OPTION_TAIL)) == NULL ){
			printf("****Error: can not find %s\n", JS_DEF_OPTION_TAIL);
			return old; /*keep old*/
		}
		optb=p1+strlen(JS_DEF_OPTION_TAIL);
		*p1='\0'; // make as available string

		/* check the content between p and p1
		 * searh all tags name as @[tags]@
		 */
		p1=option_value(optb, p);
		jsmin_c=p1; //optb;
		#ifdef DEBUG
		dumpcontent(p1,100);
		#endif //DEBUG

		return old;
	} //JS_DEF_BEGIN_TAG
	/***************************************************
	 *   support option HTML_DEF_BEGIN_TAG
	 ***************************************************/
	if( !memcmp((void *) buf, HTML_DEF_OPTION_BEGIN_TAG, strlen(HTML_DEF_OPTION_BEGIN_TAG)) )
	{
	    //printf("found <!--DEF# at %d\n", jsmin_c-jsmin_s);
		p+=strlen(HTML_DEF_OPTION_BEGIN_TAG);
		optn=p;
		/* check last end tags */
		if((p1 = strstr(p, HTML_DEF_OPTION_TAIL)) == NULL ){
			printf("****Error: can not find %s\n", HTML_DEF_OPTION_TAIL);
			return old; /*keep old*/
		}
		#ifdef DEBUG
		//dumpcontent(buf,100);
		#endif //DEBUG
		optb=p1+strlen(HTML_DEF_OPTION_TAIL);
		*p1='\0'; // make as available string

		/* check the content between p and p1
		 * searh all tags name as @[tags]@
		 */
		p1=option_value(optb, p);
		jsmin_c=p1; //optb;
		#ifdef DEBUG
		//dumpcontent(p1,100);
		#endif //DEBUG

		return old;
	} //HTML_DEF_BEGIN_TAG
   	/***************************************************
   	 *   Skip "JS_PACKER" for JSpacker next step
   	 ***************************************************/
#ifdef CONFIG_JS_PACKER
   	if ( strncasecmp( (void*)buf , JS_PACKER_BEGIN_TAG, strlen(JS_PACKER_BEGIN_TAG)) == 0 ){
	    p1=p;
	    p+=strlen(JS_PACKER_BEGIN_TAG);
	    jsmin_putc(theA);
	    while(p1 <= p){
	    	jsmin_putc(*p1++);
	    }
	    jsmin_c=p;
	    theA = jsmin_getc();
	    theB=' ';
	    return old;
	}
   	if ( strncasecmp( (void*)buf , JS_PACKER_END_TAG, strlen(JS_PACKER_END_TAG)) == 0 ){
	    p1=p;
	    p+=strlen(JS_PACKER_END_TAG);
	    jsmin_putc(theA);
	    while(p1 <= p){
	    	jsmin_putc(*p1++);
	    }
	    jsmin_c=p; // here force set point to [\n][<][s][c]
	    //theB=' ';
	    //theA=' ';
            // foce skip one more space
	    theA=' '; //\n';
	    theB='\n';
	    //printf("END_OF JS PACKER [%x]:[%x]\n",theA, theB);
	    //dumpcontent(jsmin_c,20);
	    //set_dbg=1;

	    return old;
	}
#endif //CONFIG_JS_PACKER
   }else{
   	p=(unsigned char*)buf;
   	/***************************************************
   	 *   Skip http-equiv="Content-Type" content=
   	 *   protect HTML header
   	 ***************************************************/
   	if ( strncasecmp( (void*)buf , HTML_MEDA_BEGIN_TAG, strlen(HTML_MEDA_BEGIN_TAG)) == 0 ){
	    //	*p='\n';
	    HTML_SKIP=1;
	}
   	if ( strncasecmp( (void*)buf , HTML_MEDA_END_TAG, strlen(HTML_MEDA_END_TAG)) == 0 ){
	    //	*p='\n';
	    HTML_SKIP=0;
	}

   	/***************************************************
   	 *   support option HTML_DEMO_BEGIN_TAG <!--DEMO-->
   	 ***************************************************/
   	if( !memcmp((void *) buf, HTML_DEMO_BEGIN_TAG, strlen(HTML_DEMO_BEGIN_TAG)) )
   	{
   	    	//printf("found <!--DEMO--> at %d\n", jsmin_c-jsmin_s);
   		p+=strlen(HTML_DEMO_BEGIN_TAG);
   		/* check END_DEMO*/
   		if((p1 = strstr(p, HTML_DEMO_END_TAG)) == NULL ){
   			printf("****Error: can not find %s\n", HTML_DEMO_END_TAG);
   			return old; /*keep old*/
   		}
   		p1+=strlen(HTML_DEMO_END_TAG);
   		jsmin_c=p1; /* point after HTML_DEMO_END_TAG*/
   		//printf("Drop to %d\n",jsmin_c-jsmin_s);

		#ifdef DEBUG
		dumpcontent(p1,100);
		#endif //DEBUG


   		return old;
   	} //HTML_DEMO_END_TAG
   	/***************************************************
   	 *   support option HTML_REAL_BEGIN_TAG
   	 ***************************************************/
   	if( !memcmp((void *) buf, HTML_REAL_BEGIN_TAG, strlen(HTML_REAL_BEGIN_TAG)) )
   	{
   	    	//printf("found <!--REAL at %d\n", jsmin_c-jsmin_s);
   		p+=strlen(HTML_REAL_BEGIN_TAG);
   		/* check REAL*/
   		if((p1 = strstr(p, HTML_REAL_END_TAG)) == NULL ){
   			printf("****Error: can not find %s\n", HTML_REAL_END_TAG);
   			return old; /*keep old*/
   		}
   		*(p-1)=(unsigned char) ('\n'); /* strip as new line*/
   		jsmin_c=p-1;/* set pointer to zero, after REA? */
   		memcpy(p1,"<!--",4);

		#ifdef DEBUG
		dumpcontent(p,100);
		#endif //DEBUG


   		return old;

   	} //HTML_REAL_BEGIN_TAG

	/***************************************************
	 *   support option HTML_OPTION_BEGIN_TAG
	 ***************************************************/
	if( !memcmp((void *) buf, HTML_OPTION_BEGIN_TAG, strlen(HTML_OPTION_BEGIN_TAG)) )
	{
		//printf("found HTML <!--OPT# at %d\n", jsmin_c-jsmin_s);
		p+=strlen(HTML_OPTION_BEGIN_TAG);
		optn=p;
		/* check END_OPT*/
		if((p1 = strstr(p, HTML_OPTION_END_TAG)) == NULL ){
			printf("****Error: can not find %s\n", HTML_OPTION_END_TAG);
			return old; /* keep old*/
		}
		// NOTE: 2013/4/25 support recursive OPTION
		// recursive look check if find others HTML_OPTION_BEGIN_TAG
		casecade=0;
		pp=p;
		//dumpcontent(p1,30);
		//p1+=strlen(HTML_OPTION_BEGIN_TAG);
		while(*pp)
		{
		    if((p2 = strstr(pp, HTML_OPTION_BEGIN_TAG)) != NULL )
		    {
				// if any <!--OPT exist, check if p2 < p1 then we need to skip it
            	if(p2 < p1)
            	{
					//printf("****find: rescursive find %s\n", HTML_OPTION_BEGIN_TAG);
					pp=p2+strlen(HTML_OPTION_BEGIN_TAG);
					p1+=strlen(HTML_OPTION_BEGIN_TAG);
					casecade++;
					if((p1 = strstr(p1, HTML_OPTION_END_TAG)) == NULL ){
						printf("****Error: can not find %s\n", HTML_OPTION_END_TAG);
						return old; /* keep old*/
					}
				}else{
					break;
				}
			}else{
				break;
			}
		}
		if(!*pp){
			printf("****Error: can not find %s\n", HTML_OPTION_END_TAG);
			return old; /* keep old*/
		}
		//skip thise casecade
		// no body find <!--OPT# p1 is at the END_OPT
		//printf("****after casecade [%d] find \n", casecade);
		//dumpcontent(p1,30);

		/* check OPT#, or OPT!#  */
		if(!memcmp(OPT_NOT_DEFINE, p, strlen(OPT_NOT_DEFINE)))
		{
			inverse = 1;
			optn += strlen(OPT_NOT_DEFINE);
		}
		else if (!memcmp(OPT_DEFINE, p, strlen(OPT_DEFINE)))
		{
			inverse = 0;
			optn += strlen(OPT_DEFINE);
		}
		else
		{
			printf("****Error: % has not correct format\n", HTML_OPTION_BEGIN_TAG);
			return old; /*keep old*/
		}
		/* give string an end */
		if((optb = strstr(optn, HTML_OPTION_TAIL)))
		{
			*optb = '\0';
			optb += strlen(HTML_OPTION_TAIL);
		}
		else
		{
			printf("****Error: can not find end of option %s\n",JS_OPTION_TAIL);
			return old; /*keep old*/
		}
		//printf("%s HTML OPT=%s\n", inverse?"NOT DEF": "DEF", optn);
		jsmin_c=optb;
		if(!option_match(optn, 0)^inverse)
		{
			/* no mtach skip to HTML_OPTION_END_TAG*/
			#ifdef DEBUG
			printf("%s no match %s\n",HTML_OPTION_BEGIN_TAG,optn);
			#endif
			p1+=strlen(HTML_OPTION_END_TAG);
			*(p1-1)=(unsigned char) ('\n'); /* strip as new line, why? because jsmin will skip \n */
			jsmin_c=p1-1;
			//printf("Drop to %d\n",jsmin_c-jsmin_s);
			return old; /*keep old*/
		}
		/* over write <!--END_OPT to <!--??????*/
		memcpy(p1,"<!--???????",11);

		#ifdef DEBUG
		dumpcontent(p1,100);
		#endif //DEBUG


		return old;
	}
	/***************************************************
	 *   support option HTML_DEF_BEGIN_TAG
	 ***************************************************/
	if( !memcmp((void *) buf, HTML_DEF_OPTION_BEGIN_TAG, strlen(HTML_DEF_OPTION_BEGIN_TAG)) )
	{
	    //printf("found <!--DEF# at %d\n", jsmin_c-jsmin_s);
		p+=strlen(HTML_DEF_OPTION_BEGIN_TAG);
		optn=p;
		/* check last end tags */
		if((p1 = strstr(p, HTML_DEF_OPTION_TAIL)) == NULL ){
			printf("****Error: can not find %s\n", HTML_DEF_OPTION_TAIL);
			return old; /*keep old*/
		}
		#ifdef DEBUG
		//dumpcontent(buf,100);
		#endif //DEBUG

		optb=p1+strlen(HTML_DEF_OPTION_TAIL);
		*p1='\0'; // make as available string

		/* check the content between p and p1
		 * searh all tags name as @[tags]@
		 */
		p1=option_value(optb, p);
		jsmin_c=p1; //optb;

		#ifdef DEBUG
		dumpcontent(p1,100);
		#endif //DEBUG

		return old;
	} //HTML_DEF_BEGIN_TAG

   } //old ==1, 1 means JS mode

#ifdef SUPPORT_DYNAMIC_JS
   if(F_JS_OPTIM) return 1;
   if(  strncasecmp((const char*) buf, (const char*)HTML_SCRIPT_BEGIN_TAG, strlen(HTML_SCRIPT_BEGIN_TAG))==0) {
       if(old==1){
            printf("****Error: <script :\n");
            for(i=0; i < 45;i++){ printf("%c",buf[i]);};printf("\n");
       }
       in_script++;
       //printf("\nJS ON %d, old=%d\n",in_script, old);
       /* NOTE:
        * we need keep the previous char before "<script"
        * because if previous character is space, JSMIN will drop it.
        * we set JS_READY=0 means still use non-JSMIN parsing--it will show the "space" char.
        * then after jsmin_putc(), we turn it ON.
        */
       if(old==0) JS_READY=0;
       old=1;
       //dumpcontent(buf, 45);
   }
   if( strncasecmp((const char*) buf, (const char*)HTML_SCRIPT_END_TAG, strlen(HTML_SCRIPT_END_TAG))==0) {
       if(old==0){
            printf("****Error: </script>: %d, old=%d\n",in_script, old);
	    #ifdef DEBUG
	    dumpcontent(buf,45);
	    #endif //DEBUG
        }
       theB='\n'; /* reset old valus*/
       in_script--;
       if(in_script==0)old=0;
       //printf("\nJS OFF(%d, old=%d)[%x]:[%x]\n",in_script, old, theA,theB);
       #ifdef DEBUG
       dumpcontent(buf, 45);
       #endif
   }
#endif
#ifdef SUPPORT_REMOVE_HTML_COMMENT
    /* old=0, means use mixed HTML parse
     * we also skip those contents between <!-- and -->.
     */
   if(old ==0){
        p=(unsigned char*)buf;

        /* check if not <!--# or <!--# or <!--OPT or <!--[if or*/
	if( !memcmp((void *) buf, HTML_COMMENT_BEGIN_TAG, strlen(HTML_COMMENT_BEGIN_TAG)) &&
	     memcmp((void *) buf, DOCGI_BEGIN_TAG, strlen(DOCGI_BEGIN_TAG)) &&
	     memcmp((void *) buf, IECOM_BEGIN_TAG, strlen(IECOM_BEGIN_TAG)) &&
	     memcmp((void *) buf, NONIECOM_BEGIN_TAG, strlen(NONIECOM_BEGIN_TAG)) &&
	     memcmp((void *) buf, NONIECOM_END_TAG, strlen(NONIECOM_END_TAG)) &&
	     memcmp((void *) buf, HTML_OPTION_BEGIN_TAG, strlen(HTML_OPTION_BEGIN_TAG)) )
	{
	        //printf("** HTML <!-- skip from %d \n", jsmin_c-jsmin_s);
		/* skip all <!--# tags to -->*/
		p+=strlen(HTML_COMMENT_BEGIN_TAG);
		i=0;
		while(i < (jsmin_len-(jsmin_c-jsmin_s)) )
		{
		    	// <!--#
			if( !memcmp((void *)( p+i), DOCGI_BEGIN_TAG, strlen(DOCGI_BEGIN_TAG)) ) {
			  /* found a <!--# tags*/
			  i+=strlen(DOCGI_BEGIN_TAG);
			  if((p1 = strstr((p+i), DOCGI_END_TAG)) == NULL ){
				printf("****Error: can not find %s\n", HTML_COMMENT_END_TAG);
				return old; /*keep old*/
			  }
			  i=(int)(p1-p)+strlen(DOCGI_END_TAG);

			  continue;
			}

			// check "<!-->" to "<!--"
			if( !memcmp((void *)( p+i), NONIECOM_BEGIN_TAG, strlen(NONIECOM_BEGIN_TAG)) ) {
			  // found a <!--[if tags (IE only)
			  //printf("get <!-->..\n");
			  i+=strlen(NONIECOM_BEGIN_TAG);
			  if((p1 = strstr((p+i), NONIECOM_END_TAG)) == NULL ){
				printf("****Error: can not find %s\n", NONIECOM_END_TAG);
				return old; //keep old
			  }
			  i=(int)(p1-p)+strlen(NONIECOM_END_TAG);
			  continue;
			}
			// chekc "<!--[if IE8 .." , to "<![endif]-->"
			if( !memcmp((void *)( p+i), IECOM_BEGIN_TAG, strlen(IECOM_BEGIN_TAG)) ) {
			  /* found a <!--[if tags (IE only)*/
			  //printf("get <!--[if ?? ..\n");
			  i+=strlen(IECOM_BEGIN_TAG);
			  if((p1 = strstr((p+i), IECOM_END_TAG)) == NULL ){
				printf("****Error: can not find %s\n", IECOM_END_TAG);
				return old; /*keep old*/
			  }
			  i=(int)(p1-p)+strlen(IECOM_END_TAG);
			  //printf("drop from...\n");
			  //dumpcontent(p, (p1-p));
			  continue;
			}

			// Special for "<!--" to "-->"
			// i is size
			if( !memcmp((void *)( p+i), HTML_COMMENT_END_TAG, strlen(HTML_COMMENT_END_TAG))) {
		      		/* found a --> tags*/
		      		//printf("get (%d)<-- to  --> (%d)\n",(jsmin_c-jsmin_s),(i+jsmin_c-jsmin_s)+strlen(HTML_COMMENT_BEGIN_TAG));
		      		//dumpcontent(p, 50);
		      		//i+=strlen(HTML_COMMENT_END_TAG);
		      		p+=i;
		      		break;
			}
			i++;
	    }
	    //printf("i=%d\n",i);
	    if(i > jsmin_len)
	    {
		printf("****Error: can not find %s\n", HTML_COMMENT_END_TAG);
		return old; /*keep old*/
	    }
	    if((p-jsmin_s) > jsmin_len){
		    jsmin_c=jsmin_s+jsmin_len; /* skip to last*/
		    printf("****Error: can not find %s, overflow\n", HTML_COMMENT_END_TAG);
		    return old; /*keep old*/
	    }
	    p+=strlen(HTML_COMMENT_END_TAG);

	    *(p-1)=(unsigned char) ('\n'); /* strip as new line, why? because jsmin will drop \n */
	    jsmin_c=p-1;
	    //printf("** HTML ok-> skip from %d to %d.\n", buf-jsmin_s, jsmin_c-jsmin_s);
	    #ifdef DEBUG
	    dumpcontent(p,jsmin_c-buf);
	    #endif //DEBUG

	}

   }
#endif
   return old;
}
/* get -- return the next character from stdin. Watch out for lookahead. If
        the character is a control character, translate it to a space or
        linefeed.
*/
static void jsmin_putc(int chr)
{
   if(chr != EOF ){
     if(chr !=0){
       jsmin_e[0]=chr&0xFF;
       jsmin_e++;
     }else{
       //printf("drop zero\n");
     }
    /*
     * if JS_MIN is ON then JS_READY is ON
     */
     JS_READY=JS_OPTIM;
     //printf("[%c]",chr);
//     printf("[%c- %d, %d /%d]",chr, (int)(jsmin_c - jsmin_s), (int)(jsmin_c - jsmin_s), jsmin_len);
   }
   return;
}
static int jsmin_getc()
{
   int rc=EOF;
   if( (int)(jsmin_c - jsmin_s) >= jsmin_len){
       rc=EOF;
   }else{
     if(!jsmin_off){
       /* off jsmin no do any options*/
       JS_OPTIM=isparseSCRIPT(jsmin_c ,JS_OPTIM);
     }
     rc=(unsigned char)jsmin_c[0];
     theLast=rc;
     jsmin_c++;
   }
   //printf("<%d>",JS_OPTIM);
   return rc;
}
//static int get(FILE *infp)
static int get()
{
    int c = theLookahead;
    theLookahead = EOF;
    if (c == EOF) {
        c = jsmin_getc(); //getc(infp);
    }
/*
    if( c >= 32)
    	printf("{%c}",c);
    else
    	printf("{%02x}",c);
*/
    if (c >= ' ' || c == '\n' || c == EOF) {
        return c;
    }
    if (c == '\r') {
        if (jsmin_tag_pre != 4) {
            return '\n';
        }
    }
    if(c ==0)
       return 0; /* special case, which mean some content been drop by jsmin_getc(), we need set it zero */

    return ' ';

}


/* peek -- get the next character without getting it.
*/

//static int peek(FILE *infp)
static int peek()
{
    theLookahead = get();
    return theLookahead;
}


/* next -- get the next character, excluding comments. peek(infp) is used to see
        if a '/' is followed by a '/' or '*'.
*/

//static int next(FILE *infp,FILE *outfp)
static int next()
{

    int c = get();

    if(jsmin_off || !JS_READY)
        return c;

    if  (c == '/') {
        switch (peek()) {
        case '/':
            for (;;) {
                c = get();
                if (c <= '\n') {
                    return c;
                }
            }
        case '*':
            get();
            for (;;) {
                switch (get()) {
                case '*':
                    if (peek() == '/') {
                        get();
                        return ' ';
                    }
                    break;
                case EOF:
                    fprintf(stderr, "\nError: JSMIN Unterminated comment /* in %s.\n",IN_FILE);
		    c=EOF;
		    return c;
                    //break;
                }
            }
        default:
            return c;
        }
    }
    return c;
}


/* action -- do something! What you do is determined by the argument:
        1   Output A. Copy B to A. Get the next B.
        2   Copy B to A. Get the next B. (Delete A).
        3   Get the next B. (Delete B).
   action treats a string as a single character. Wow!
   action recognizes a regular expression if it is preceded by ( or , or =.
*/

//static void action(int d, FILE *infp, FILE *outfp)
static void action(int d)
{
	int theP=0;
	#ifdef DEBUG
	//printf(" a[%d],theA=%c(%x),theB=%c(%x)\n",d,theA,theA,theB,theB);
	#endif
	switch (d) {
	case 1:
		//printf("action:1\n");
		jsmin_putc(theA);
	case 2:
		theA = theB;
		//printf("action:2 A:[%x]:[%x] %d: %d\n",theA,theB, jsmin_off, (JS_OPTIM && JS_READY));
		if(jsmin_off || !(JS_OPTIM && JS_READY) || (jsmin_tag_pre >= 4))
		{
			/* when JSOFF is enable, we no need to do the JS optimized task, just get next character.*/
			theB = next();
			break;
		}

		// for HTML no need parsing ' or "
		if (theA == '\'' || theA == '"') {
			for (;;) {
				jsmin_putc(theA);
				theP = theA;
				theA = get();
				if (theA == theB) {
					//printf("-- >%c, %c\n",theA, theB);
					break;
				}

				if (theA == '\\') {
					//putc(theA, outfp);
					jsmin_putc (theA);
					theA = get();
				}

				if (theA == EOF) {
					printf("Error: JSMIN unterminated string literal in %s.\n(Check the quotation marks[%c], they should be pair)\n",IN_FILE, theB);
					#ifdef DEBUG
					printf("\nERROR=====================\n");
					dumpcontent((char *)(jsmin_e - 30), 30);
					printf("\n=======================END\n");
					#endif
					break;
				}
			}
		}

		if(HTML_SKIP){
			// force output after ", even it is space
			jsmin_putc(theB);
			theA = next();
		}
	case 3:
		//printf("action:3 [%x]:[%x] %d: %d\n",theA, theB,jsmin_off, (JS_OPTIM && JS_READY));
		theB = next();

		if (theB == '/') {
			switch (theA) {
			case '(':
			case ',':
			case '=':
			case ':':
			case '[':
			case '!':
			case '&':
			case '|':
			case '?':
			case '{':
			case '}':
			case ';':
			case '\n':
				jsmin_putc(theA);
				jsmin_putc(theB);
				//putc(theA, outfp);
				//putc(theB, outfp);
				for (;;) {
					theA = get();
					if (theA == '/') {
						break;
					}

					if (theA =='\\') {
						//putc(theA, outfp);
						jsmin_putc(theA);
						theA = get();
					}

					if (theA == EOF) {
						fprintf(stderr,"Error: JSMIN unterminated Regular Expression literal in %s.\n",IN_FILE);
						break;
					}

					//putc(theA, outfp);
					jsmin_putc(theA);
				}
				theB = next();
			}
		}
	}
}

/* jsmin -- Copy the input to the output, deleting the characters which are
        insignificant to JavaScript. Comments will be removed. Tabs will be
        replaced with spaces. Carriage returns will be replaced with linefeeds.
        Most spaces and linefeeds will be removed.
*/
static int
jsmin(unsigned char *buf, int len)
{
	jsmin_s=jsmin_c=jsmin_e=buf;
	jsmin_len=len;
	int i;

#if 0 //def DEBUG
	printf("\nBEGIN**********\n");
	dumpcontent(buf, len);
	printf("\n**************\n");
#endif
	theA = '\n';
	if (!jsmin_off) {
		action(3);
	}
	else {
		theB = next();
	}

	// action(2);
	while ((theA != EOF)){
		if( (int)(jsmin_c - jsmin_s) > jsmin_len)
			break;

		if(jsmin_off){ /* off jsmin only drop comment and options CGI tags*/
			action(1);
			continue;
		}

		if (jsmin_tag_pre == 4) {
			action(1);
			continue;
		}

		if (theA == 0) {
			theA = theB;
			theB = next();
		}

		switch (theA) {
		case ' ':
			//printf("{ } [%x : %c]\n",(theA)? theA:'?', (theB)? theB:'?');
			if (JS_OPTIM && JS_READY) {
				if (isAlphanum (theB)) {
					action(1);
				}
				else{
					action(2);
				}
			}
			else {
				/* mix JS and HTML mode*/
				if (isAlphanum(theB)) {
					action(1);
				}
				else if (theB == '<' || theB == '&' || theB == ')' || theB == '(' || theB == '+') {
					action(1);
				}
				else {
					action(2);
				}
			}
			break;
		case '\n':
			//printf("{cr} [%x : %c]\n",(theA)? theA:'?', (theB)? theB:'?');
			switch (theB) {
			case '{':
			case '[':
			case '(':
			case '+':
			case '-':
				action(1);
				break;
			case ' ':
				action(3);
				break;
			default:
				//printf("#");
				if (isAlphanum(theB)) {
					action(1);
				}
				else {
					#ifdef ARCADYAN_PATCH
					if (g_keep_js_nl && theB == '<')
						action(1);
					else
					#endif //ARCADYAN_PATCH
						action(2);
				}
			}
			break;

		default:
			switch (theB) {
			case ' ':
				//printf("(+3.1)");
				if (isAlphanum(theA)) {
					action(1);
					break;
				}

				if(JS_OPTIM  && JS_READY)
				{
					action(3);
				}
				else {
					/* mix JS and HTML mode*/
					switch (theA) {
					case '>':
						if (jsmin_tag_pre == 3) {
							jsmin_tag_pre = 4;
						}
						else if (jsmin_tag_pre != 4) {
							jsmin_tag_pre = 0;
						}

					case '%':
					case ',':
					case '.':
					case '&':
					case ';':
					case ')':
					case '(':
					case '+':
					case '\"':
					case '\'':
					case '\t':
						action(1);
						break;
					default:
						action(3);
					}
				}
				break;
			case '\n':
				//printf("(+3.2)");
				switch (theA) {
				case '}':
				case ']':
				case ')':
				case '+':
				case '-':
				case '\"':
				case '\'':
					action(1);
					break;
				default:
					if (isAlphanum(theA)) {
						action(1);
					} else {
						if (theA == '>') {
							if (jsmin_tag_pre == 3) {
								jsmin_tag_pre = 4;
							}
							else if (jsmin_tag_pre != 4) {
								jsmin_tag_pre = 0;
							}
						}

						#ifdef ARCADYAN_PATCH
						if(g_keep_js_nl)
							action(1);
						else
						#endif //ARCADYAN_PATCH
							action(3);
					}
				}
				break;
			default:
				if (theA == '<') {
					if (jsmin_tag_pre == 0 && (theB == 'p' || theB == 'P')) {
						jsmin_tag_pre = 1;
					}
				}
				else if (theA == '/') {
					if (jsmin_tag_pre == 4 && (theB == 'p' || theB == 'P')) {
						jsmin_tag_pre = 5;
					}
				}
				else {
					if (jsmin_tag_pre == 1 && (theB == 'r' || theB == 'R')) {
						jsmin_tag_pre = 2;
					}
					else if (jsmin_tag_pre == 2 && (theB == 'e' || theB == 'E')) {
						jsmin_tag_pre = 3;
					}
					else if (jsmin_tag_pre == 5 && (theB == 'r' || theB == 'R')) {
						jsmin_tag_pre = 6;
					}
					else if (jsmin_tag_pre == 6 && (theB == 'e' || theB == 'E')) {
						jsmin_tag_pre = 7;
					}
					else if (jsmin_tag_pre != 3 && jsmin_tag_pre != 7) {
						jsmin_tag_pre = 0;
					}
				}

				action(1);
				break;
			}
		}
	}
	// force last character is NULL
	*(jsmin_e)='\0';
#if 0 //def DEBUG
	printf("\n RESULT=====================\n");
	dumpcontent(buf, jsmin_e-jsmin_s);
	printf("=======================END\n");
#endif

	return ((int) (jsmin_e-jsmin_s));
}

#endif /* END of CONFIG_JSMIN */

#ifdef CONFIG_JS_PACKER
#define JS_IN_TMP "/tmp/.jsin.tmp"
#define JS_OUT_TMP "/tmp/.jsout.tmp"
static int jspacker(unsigned char *buf, int len)
{
    char cmd[1024];
	char JS_IN_FILE[]=JS_OUT_TMP  ".XXXXXX";
	char JS_OUT_FILE[]=JS_OUT_TMP ".XXXXXX";

    FILE *jsfp;
    struct stat 	st;
    int size;
    unsigned char *end_buf=buf+len;
    unsigned char *p1, *p=buf;
    unsigned char *b,*e,*t;
    int c;
    int rtn_len=0;
    int shift_sz,sz=0;
	int fd;

    // generate a tempary file for read/write purpose!
    // here we use mkstemp, but we ONLY want the NAME!!!
	fd=mkstemp(JS_IN_FILE);
	if(fd==-1){
		printf("system error\n");
		return rtn_len;
	}
	close(fd);
	unlink(JS_IN_FILE);

	fd=mkstemp(JS_OUT_FILE);
	if(fd==-1){
		printf("system error\n");
		return rtn_len;
	}
	close(fd);
	unlink(JS_OUT_FILE);


    while(p <= end_buf && *p){
	// if JS_PACKER
   	if ( strncasecmp( (void*)p , JS_PACKER_BEGIN_TAG, strlen(JS_PACKER_BEGIN_TAG)) == 0 ){
	    b=p+strlen(JS_PACKER_BEGIN_TAG);
	    if((p1 = strstr(p, JS_PACKER_END_TAG)) == NULL ){
		printf("****Error: %s\n",JS_PACKER_END_TAG);
		// drop previous tags
		sz=len-(b-buf);
		memcpy(p,b,sz);
		*(p+sz)='\0';
		rtn_len=len-strlen(JS_PACKER_BEGIN_TAG);
		return rtn_len;
	    }
	    e=p1+strlen(JS_PACKER_END_TAG);
	    // do encry
	    if ((jsfp = fopen(JS_IN_FILE, "w")) != NULL ) {
		//memcpy(p,"/*HS",4);
		//memcpy(p1,"/*OK",4);
		fwrite((const void *)b, (size_t) (p1-b), 1, jsfp);
		fclose(jsfp);
		// here g_jspacker_encode is 1 means we support encryption mode
		// need send parameters -e62
		sprintf(cmd,"/usr/bin/perl %s -i %s -o %s -q %s",g_jspacker, JS_IN_FILE,JS_OUT_FILE, (g_jspacker_encode)?"-e62":"");
		system(cmd);
		if(!stat(JS_OUT_FILE, &st))
		{
		    size=(int)st.st_size;
		    if (size)
		    {
			// NOTE:
			//   it may no happen, but need to check for safe
			if(size > (e-p)){
			    //p  b       p1 e     end_buf
			    //+--+-------+--+-----------+
			    //|<--size ------->|
			    shift_sz=size-(e-p);
			    t=end_buf;
/*
			    printf("size[%d] [p=%d to e=%d:%d]\n",size, p-buf, e-buf, e-p);
			    printf("[%s] from %d, shift [%d]\n",e, e-buf,shift_sz);
			    printf("move from %d to %d\n",end_buf-buf, end_buf-buf+shift_sz);
*/
			    while(t >=e)
			    {
				*(t+shift_sz)=*(t);
				t--;
			    }
			    end_buf+=shift_sz;
			    e+=shift_sz;

			}
			jsfp = fopen(JS_OUT_FILE, "r");
			fread(p, size, 1, jsfp);
			fclose(jsfp);
			memcpy(p+size,e, (end_buf-e+1));
			end_buf=p+size+(end_buf-e+1);
			*(end_buf)='\0';
			*(end_buf+1)='\0';
			e=p+size; //set to next block
			//printf("last e[%d][%s]\n",p+size-buf,  p+size);
		    }
		}else{
		    printf("stat %s err \n", JS_OUT_FILE);
    		}
	    }else{
		fprintf(stderr, "Can't open out file: [%s]\n",JS_IN_FILE);
	    }
	    p=e-1; // shift to next block, but back one character
    	}
    	p++;
    }
    unlink(JS_IN_FILE);
    unlink(JS_OUT_FILE);
    rtn_len=p-buf;
    return rtn_len;
}
#endif //CONFIG_JS_PACKER
#ifdef CONFIG_CSS_PACKER
static int
cssmin(unsigned char *buf, int len)
{
    unsigned char *cssmin_s,*cssmin_c,*cssmin_e;
    int cssmin_len=len;
    int skip_sz;
    unsigned char *skip;
    int i;
    cssmin_s=cssmin_c=buf;
    cssmin_e=buf+cssmin_len-1;
    printf("check CSS size: %d\n", cssmin_len);
    while(cssmin_c < cssmin_e){
        if(*cssmin_c == '/' && *(cssmin_c+1) =='*')
	{
	    skip=cssmin_c+2;
           // printf( "find [%d]\n",cssmin_c - cssmin_s);
    	    while(skip < cssmin_e){
		if(*skip== '*' && *(skip+1)== '/')
		{
			//we needskip those
			//  12345678901234567890
			//  /* ?????        */
			//  ^__cssmin_c     ^___ skip
			//  where cssmin_c(1) <-- skip+2(19)
			//
			skip_sz=skip-cssmin_c+2;
			//fprintf(stderr, "skip [%d] ",skip_sz);
			cssmin_e-=skip_sz;
			//fprintf(stderr, "%d jump to [%d] skip[%d] , last size=[%d]\n",cssmin_c-cssmin_s,skip - cssmin_s+2, skip_sz, cssmin_len);
			//dumpcontent(skip+2,50);
			memcpy(cssmin_c,skip+2,cssmin_e-cssmin_c);
			*(cssmin_e)='\0';
			break;
		}
		skip++;
            }
            if(skip >= cssmin_e){
            	//printf("Error [%d]\n",skip-cssmin_s);
		cssmin_c++;
            }
            continue;
        } else if(((*cssmin_c==':') && (*(cssmin_c+1)==' ')) ) {
            // skip :<space> to :
	    cssmin_e-=1;
	    cssmin_c++;
	    memcpy(cssmin_c,cssmin_c+1,cssmin_e-cssmin_c);
	    *(cssmin_e)='\0';
	}else if((*cssmin_c == '\n') ||
	         (*cssmin_c == '\r') ||
	         (*cssmin_c == '\t') ||
	         ((*cssmin_c==';') && (*(cssmin_c+1)=='}')) ||
	         ((*cssmin_c==' ') && (*(cssmin_c+1)==' ')) ) {
	   // skip \n \r \t
	   // skip <space><space> to <space>
	   // skip ;} to }
	   cssmin_e-=1;
 	   memcpy(cssmin_c,cssmin_c+1,cssmin_e-cssmin_c);
	   *(cssmin_e)='\0';
	}else{
	    cssmin_c++;
	}
    }
    return (cssmin_e-cssmin_s);
}
#endif
