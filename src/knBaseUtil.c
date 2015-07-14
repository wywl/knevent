/************************ knBaseUtil.c ******************************************
** CopyRight ( C )	kernel2
** Classification:	knBaseUtil.c
** Author&Date:		Xie Jianping	29/1/1999
** Reversion:		Xiaoqun Lee	 30/5/2002 2008-08-28
** Description:		2008-08-28 重写knPrintf文件。测试：内存泄露为0。
**					本程序非线程安全。
**					考虑到knPrintf模块承担的内容不仅限于Debug功能,还提供数据类
**					型,封装基础函数等功能,因此,将这个模块改名为knBaseUtil。
**					2010-05-12
**				    李家允与王雁华对本模块的Linux迁移做了大量工作。后期代码的Linux
**					封装一部分由王雁华编写的，李小群做了测试验证与规范化。
**					本部分程序验证了在Windows环境下与Linux环境下中文字符集的选用非
**					常重要，Windows Visual Studio 2008程序代码必须保存为UTF-8 无签
**					名模式，并且在Project配置中，禁用特定警告4819。Linux开发平台
**					CodeBlocks的字符集也必须用UTF-8。
**					校验函数封装由杨新峰提供,已经迁出转移到knEncryp程序文件中。
**					本程序已经能够在Linux下编译执行。2011-04-21
*******************************************************************************/
// ANSI C header files
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <knBaseUtil.h>

// Windows C header files
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <winsock.h>
#include <Dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#endif
// Linux header files
#ifdef _LINUX32
#include <iconv.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <iconv.h>
#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#endif
// kernel2 C header file


// ******************** 变量声明 **********************************************
static INT	p_knPrintfStatus		= PRINT_ON;	// knPrintf操作状态,默认PRINT_ON。
static FILE	*p_knCurrentFile		= NULL;					// 当前文件指针。
static CHAR	p_knErrorLogFile[32]	= "knError.log";		// 默认错误日志名。
static CHAR	p_knProcLogFile[32]		= "knProcess.log";		// 默认操作日志名。
static UINT p_knMaxBytesForProc		= PRINT_MAXBYTESINLOG;	// 操作日志容量。
static UINT p_knMaxBytesForError	= PRINT_MAXBYTESINLOG;	// 出错日志容量。

static INT  p_knEndianStatus = KN_ENDIAN_HOST;				// 默认主机字节序。
// ******************** End of 函数声明 ***************************************

// ******************** 函数声明 **********************************************
// 函数：获取文件长度
// 输入：void* *fp_ 文件指针。
// 返回：文件长度(bytes), -1 失败。
extern INT knGetFileSize(void *fp_);

// 函数：文件超长度处理。
// 输入：CHAR *file_ 文件名,void *fp_ 文件句柄。
// 返回：0 成功,-1 失败。
static INT knOverFileSize(const CHAR *file_, void *fp_);

// 函数：将字符串写入操作日志文件（类似fputs(),内部函数）。
// 输入：CHAR *str_：信息内容字符串; INT fileFlag_: 日志文件指示：0 操作日志文件,1 出错日志文件。
// 返回：0 成功；-1 失败。
// 注释：本模块出错信息不能存入出错日志,否则出现递归调用错误。
//		 本函数不具备线程安全保障。需要考虑优化线程安全。
extern INT knPuts(const CHAR *str_, INT fileFlag_);

// 函数：输出打印处理。(内部函数)
// 输入：INT fileFlag_：knPrintf或knError调用标识；const CHAR *fmt_：输入；va_list ap_
// 返回：0 成功；-1 失败。
extern INT knPrintfDoIt(INT fileFlag_, const CHAR *fmt_, va_list ap_);

// 函数：截取字符串标识符后的几位数.
// 输入：CHAR *str_ 字符串, char symb_ 分隔符, int pos_ 保留分隔符后的字符数量
// 返回：截取的子串.
// 注释：
extern KN_LIB_API CHAR *knCutStringPos(CHAR *str_, char symb_, int pos_);

/** 函数：封装vsprintf().
输入：CHAR *buffer_ 输出内存, const CHAR *format_ 格式定义, CHAR *argptr_ 变量列表指针.
返回：复制字符串字符数；-1 失败。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knVsprintf(CHAR *buffer_, const CHAR *format_, CHAR *argptr_);
// ***************** 字节序处理 ***********************************************
// 函数：将浮点数float从主机字节序转变为网络字节序，并保存到knFloatParts中。
// 输入：float f_ 浮点数, knFParts &p_返回浮点数网络字节序数据结构(in-out)。
// 返回：无。
extern void htonf(float f_, knFParts *p_);

// 函数：将浮点数float从网络字节序转变为主机字节序。
// 输入：knFParts p_ 浮点数网络字节序数据结构, float *f_ 返回主机字节序浮点数(in-out)。
// 返回：0 成功；-1 失败。
extern int ntohf(knFParts p_, float *f_);

// 函数：将浮点数double从主机字节序转变为网络字节序，并保存到dPart中。
// 输入：double f_ 浮点数, knFParts &p_返回浮点数网络字节序数据结构(in-out)。
// 返回：无。
extern void htond(double d_, knFParts *p_);

// 函数：将浮点数double从网络字节序转变为主机字节序。
// 输入：knFParts p_ 浮点数网络字节序数据结构, float *f_ 返回主机字节序浮点数(in-out)。
// 返回：0 成功；-1 失败。
extern int ntohd(knFParts p_, double *d_);
// ***************** End of 字节序处理 ****************************************
// ******************** End of 函数声明 ***************************************

// ******************** 打印接口API *******************************************
// 函数：打印信息输出。
// 输入：const CHAR *fmt_打印输入。
// 返回：无。
// 注释：受打印状态设置的影响。
void knPrintf(const CHAR *fmt_, ...) {
	va_list	ap;
	if (!p_knPrintfStatus) { // 条件式如下含义：p_knPrintfStatus == 0 停止打印输出。
		return;
	}
	va_start(ap, fmt_);
	if(-1 == knPrintfDoIt(PRINT_KNPRINTF, fmt_, ap)) {
		knError("%s Line %d warning: knPrintfDoIt() failed.\n", __FILE__, __LINE__);
	}
	va_end(ap);
}

// 函数：错误信息输出。
// 输入：const CHAR *fmt_ 打印输入。
// 返回：无。
// 注释：不受打印状态设置的影响。
void knError(const CHAR *fmt, ...) {
	va_list	ap;
	va_start(ap, fmt);
	if(-1 == knPrintfDoIt(PRINT_KNERROR, fmt, ap)) {
		knError("%s Line %d warning: knPrintfDoIt() failed.\n", __FILE__, __LINE__);
	}
	va_end(ap);
}

// 函数：日志信息输出。
// 输入：const CHAR *fmt_ 日志输入。
// 返回：无。
// 注释：受日志设置状态设置的影响。待开发。
void knLog(const CHAR *fmt_, ...) {
	fmt_ = NULL;
	knPrintf("knBaseUtil::knLog() TBD.\n");
}
// ******************** End of 打印接口API ************************************

// ******************** 打印设置 **********************************************
// 函数：关闭打印屏幕与操作日志文件日志输出。
// 输入：无。
// 返回：无。
void knPrintfOff(void) {p_knPrintfStatus = PRINT_OFF;}

// 函数：打开打印屏幕输出。
// 输入：无。
// 返回：无。
void knPrintfOn(void) {p_knPrintfStatus = PRINT_ON;}

// 函数：打开打印屏幕与操作日志文件日志输出。
// 输入：无。
// 返回：无。
void knPrintfToFile(void) {p_knPrintfStatus = PRINT_TO_FILE;}

// 函数：只打开打印操作文件日志输出。
// 输入：无。
// 返回：无。
void knPrintfToFileOnly(void) {p_knPrintfStatus = PRINT_TO_FILE_ONLY;}

// 函数：设置操作日志输出文件名与文件长度。
// 输入：CHAR *fname_：文件名；UINT maxbytes_：文件长度（字节）。
// 返回：0 成功；-1 失败。
INT knPrintfProcLogConf(const CHAR *fname_, const UINT bytes_) {
	if(fname_ != NULL) {
		knStrcpy(p_knProcLogFile, fname_);
	}
	else {
		knError("%s Line %d warning: fname_ is NULL.\n", __FILE__, __LINE__);
	}
	if(bytes_ > 0)
		p_knMaxBytesForProc = bytes_;
	return 0;
}

// 函数：设置错误日志输出文件名与文件长度。
// 输入：CHAR *fname_：文件名；UINT maxbytes_：文件长度（字节）。
// 返回：0 成功；-1 失败。
INT knPrintfErrorLogConf(const CHAR *fname_, const UINT bytes_) {
	if(fname_ != NULL) {
		knStrcpy(p_knErrorLogFile, fname_);
	}
	else {
		knError("%s Line %d warning: fname_ is NULL.\n", __FILE__, __LINE__);
	}
	if(bytes_ > 0)
		p_knMaxBytesForError = bytes_;
	return 0;
}

// 函数：设置打印状态。
// 输入：const INT printfStatus_：0 - 3。
// 返回：无。
void knSetPrintfStatus(const INT status_) {
	INT iStatus;
	iStatus = status_;
	if (iStatus == PRINT_OFF || iStatus == PRINT_ON || iStatus == PRINT_TO_FILE || iStatus == PRINT_TO_FILE_ONLY) {
		p_knPrintfStatus = status_;
	}
	else {
		knError("%s Line %d warnning: Illegal PrintStatus! The PrintSTDIO is forced turn to PRINT_ON.\n", __FILE__, __LINE__);
		p_knPrintfStatus = PRINT_ON;
	}
}

// 函数：获取操作日志名。
// 输入：CHAR *name_ 返回文件名(out)。
// 返回：文件名。
char *knGetProcLogName(CHAR *name_) {
	if(NULL != name_) {
		knStrcpy(name_, p_knProcLogFile);
	}
	return p_knProcLogFile;
}

// 函数：获取错误日志名。
// 输入：CHAR *name_ 返回文件名(out)。
// 返回：无。
char *knGetErrorLogName(CHAR *name_) {
	if(NULL != name_) {
		knStrcpy(name_, p_knErrorLogFile);
	}
	return p_knErrorLogFile;
}

// 函数：获取操作日志文件最大长度。
// 输入：无。
// 返回：文件长度。
INT knGetProcLogSize(void) {
	return p_knMaxBytesForProc;
}

// 函数：获取错误日志文件最大长度。
// 输入：无。
// 返回：文件长度。
INT knGetErrorLogSize(void) {
	return p_knMaxBytesForError;
}

// 函数：设置操作日志名。
// 输入：CHAR *name_ 文件名。
// 返回：无。
void knSetProcLogName(CHAR *name_) {
	knBzero(p_knProcLogFile, sizeof(p_knProcLogFile));
	knStrcpy(p_knProcLogFile, name_);
}

// 函数：设置错误日志名。
// 输入：CHAR *name_ 文件名。
// 返回：无。
void knSetErrorLogName(CHAR *name_){
	knBzero(p_knErrorLogFile, sizeof(p_knErrorLogFile));
	knStrcpy(p_knErrorLogFile, name_);
}

// 函数：设置操作日志文件最大长度。
// 输入：UINT size_ 文件长度。
// 返回：无。
void knSetProcLogSize(UINT size_){
	p_knMaxBytesForProc = size_;
}

// 函数：设置错误日志文件最大长度。
// 输入：UINT size_ 文件长度。
// 返回：无。
void knSetErrorLogSize(UINT size_) {
	p_knMaxBytesForError = size_;
}
// ******************** End of 打印设置 ***************************************

// ******************** 文件处理 **********************************************
// 函数：获取文件长度
// 输入：void *fp_ 文件指针。
// 返回：文件长度(bytes), -1 失败。
// 注释：基础函数。
INT knGetFileSize(void *fp_) {
	CHAR	buf[1024];						// 保存输入字符串信息。出错时,保存出错信息,供打印输出。
	INT		len, ret;
	memset(buf, 0, sizeof(buf));
	if(fp_ == NULL) {
		knSprintf(buf, "knBaseUtil::knGetFileSize() warning: Input fp_ is NULL.\n", __FILE__, __LINE__);
		fputs(buf, stdout);
		return -1;
	}
	ret = fseek((FILE*)fp_, 0, SEEK_END);	// 定位到流文件尾。
	len = ftell((FILE*)fp_);						// 获取流文件长度。
	return len;
}

// 函数：文件超长度处理。
// 输入：CHAR* 文件名,void* 文件句柄。
// 返回：0 成功,-1 失败。
INT knOverFileSize(const CHAR *file_, void *fp_) {
	CHAR str[PRINT_INPUTMAXBYTES];	// 保存输入字符串信息。出错时,保存出错信息,供打印输出。
	CHAR fname[128];			// 文件名。
	CHAR temf[128];				// 暂时文件名（token后被去掉文件后缀）。
	CHAR arcf[128];				// 存档文件（增加时间标签）。
	CHAR sysDate[17];			// 系统日期。
	INT	 ret = -1;				// 返回。
	CHAR *context;				// 用于knStrtok()。

	memset(str, 0, PRINT_INPUTMAXBYTES);
	memset(fname, 0, sizeof(fname));
	memset(temf, 0, sizeof(temf));
	memset(arcf, 0, sizeof(arcf));
	memset(sysDate, 0, sizeof(sysDate));

	knStrcpy(fname, file_);
	fclose((FILE*)fp_);									// 关闭当前文件。
	knStrcpy(temf, file_);							// 将当前文件名存入暂存变量中。
	knStrtok(temf, ".", &context);					// 去掉文件名后缀(.log)。
	knSysDateTime(2, sysDate);						// 获取系统日期(yyyy=mm-dd hh:mm)。
	knFilterChar(sysDate, ':');						// 删除系统日期中的hh:mm中的“:”符号，文件名不允许上述符号。
	knSprintf(arcf, "%s_%s.log", temf, sysDate);	// 组成新文件名,例如,knError_2008-12-02.log。
	ret = rename(fname, arcf);						// 当前文件名改成加时间标签的档案文件。
	if(ret == -1) {
		knSprintf(str, "knBaseUtil::knOverFileSize() warning: rename() the current %s file to %s archive file failed.\n", fname, arcf);
		fputs(str, stdout);
	}
	return ret;
}

// 函数：输出打印处理。(内部函数)
// 输入：INT fileFlag_：knPrintf或knError调用标识；const CHAR *fmt_：输入；va_list ap_
// 返回：0 成功；-1 失败。
INT knPrintfDoIt(INT fileFlag_, const CHAR *fmt_, va_list ap_) {
	CHAR buf[PRINT_INPUTMAXBYTES];	// 定义内存数组。出错时,保存出错信息,供打印输出。
	INT len = PRINT_INPUTMAXBYTES;
	memset(buf, 0, len);
	if(-1 == knVsprintf(buf, fmt_, ap_)) {
		knError("%s Line %d warning: knVsprintf() failed,\n", __FILE__, __LINE__);
		return -1;
	}
	p_knCurrentFile = stdout;
	if((INT)strlen(buf) >= len) { // 内存数组长度保护。
		knSprintf(buf, "knBaseUtil::knPrintfDoIt() warnning: The length of Input string is %d bytes, It is more than %d bytes. It is too long to process.\n", strlen(buf), len);
		fputs(buf, stdout);
		// 本域曾出现严重Bug。lxq 200903011
		return -1;
	}
	if(PRINT_KNPRINTF == fileFlag_) {	// knPrintf调用的处理逻辑。
		if(p_knPrintfStatus == PRINT_ON || p_knPrintfStatus == PRINT_TO_FILE) {
			fputs(buf, p_knCurrentFile);
			fflush(p_knCurrentFile);
		}
		if(p_knPrintfStatus == PRINT_TO_FILE || p_knPrintfStatus == PRINT_TO_FILE_ONLY)
			knPuts(buf, PRINT_KNPRINTF);
		}
	else if(PRINT_KNERROR == fileFlag_) {	// knError调用的处理逻辑。
		if(p_knPrintfStatus == PRINT_ON || p_knPrintfStatus == PRINT_TO_FILE) {
			fputs(buf, p_knCurrentFile);
			fflush(p_knCurrentFile);
		}
		// knError调用时,在p_knPrintfStatus是以下设置时,也要输出到操作日志中。
		// 即两次写日志,写操作日志,同时写出错日志。
		//if(p_knPrintfStatus == PRINT_TO_FILE || p_knPrintfStatus == PRINT_TO_FILE_ONLY)
			//knPuts(buf, PRINT_KNPRINTF);
		knPuts(buf, PRINT_KNERROR);	// 出错日志是不能被写文件开关屏蔽的。
	}
	return 0;
}

// 函数：将字符串写入操作日志文件（类似fputs(),内部函数）。
// 输入：CHAR *str_：信息内容字符串; INT fileFlag_: 日志文件指示：0 操作日志文件,1 出错日志文件。
// 返回：0 成功；-1 失败。
// 注释：本模块出错信息不能存入出错日志,否则出现递归调用错误。
//		 本函数不具备线程安全保障。需要考虑优化线程安全。
INT knPuts(const CHAR *str_, INT fileFlag_) {	// 成功返回0；失败返回-1；
	INT	 len;					// 文件长度。
	CHAR fname[128];			// 文件名。
	CHAR str[PRINT_INPUTMAXBYTES];	// 保存输入字符串信息。出错时,保存出错信息,供打印输出。
	CHAR str1[PRINT_INPUTMAXBYTES];	// 保存输入字符串信息+系统日期时间。
	FILE *fp = NULL;			// 文件句柄。
	INT	 i = 0;					// 计数器(兼表示是否有“\n”断行符号)。
	INT	 maxBytes;				// 文件长度上限。
	CHAR sysDateTime[32];		// 系统日期时间。

	memset(fname, 0, sizeof(fname));
	memset(str, 0, sizeof(str));
	memset(str1, 0, sizeof(str1));
	memset(sysDateTime, 0, sizeof(sysDateTime));

	if(fileFlag_ == PRINT_KNPRINTF) {
		knSprintf(fname, "%s", p_knProcLogFile);// 将操作操作日志文件名存入fname。
		maxBytes = p_knMaxBytesForProc;			// 操作日志文件长度。
	}
	else {
		knSprintf(fname, "%s", p_knErrorLogFile);	// 将出错志文件名存入fname。
		maxBytes = p_knMaxBytesForError;		// 出错日志文件长度。
	}

	fp = (FILE*)knFopen(fname, "a");					// 打开文件,追加内容。lxq20081214
	if(fp == NULL) {
		knSprintf(str, "knBaseUtil::knPuts() Warning: knFopen(%s) failed.\n", fname);
		fputs(str, stdout);
		return -1;
	}
	len = knGetFileSize(fp);
	knSysDateTime(0, sysDateTime);				// 获取系统日期时间(yyyy-mm-dd hh:mm:ss)。

	// 日志文件长度超限处理。
	if(len > maxBytes) {
		if(knOverFileSize(fname, fp) == -1) {
			knSprintf(str, "knBaseUtil::knPuts() warning: knOverFileSize() processing %s file failed.\n", fname);
			fputs(str, stdout);
			return -1;
		}
		fp = (FILE*)knFopen(fname, "a");				// 重新创建fname文件,追加内容。
		if(fp == NULL) {
			knSprintf(str, "knBaseUtil::knPuts() warning: knFopen() %s file error.\n", fname);
			fputs(str, stdout);
			return -1;
		}
	}
	// 检测str_包含的字符串是否有结束符“\n”,如果没有,补充“\n”。
	for(i = 0; i < (INT)strlen(str_); i++) {
		if(str_[i] == '\n') {
			i = -1;								// 改变i定义赋值,表示输入数据中有"\n"。
			break;
		}
	}
	if(i == -1) knSprintf(str, "%s", str_);		// 将str_的内容原封不动复制到str中。
	else knSprintf(str, "%s\n", str_);			// 将str_的内容加上"\n"后复制到str中。

	knSprintf(str1, "%s|| %s", sysDateTime, str);// 将系统日期时间与str变量内容存入str1中。

	//knPrintf("knPuts()提示: 处理后的字符串是：%s (length:%d)\n", str1, strlen(str1)); // 测试点。
	fputs(str1, fp);							// 将str内容输入到流文件中。
	fclose(fp);									// 关闭打开的文件。追加内容。可增加安全锁定解锁。lxq20081214
	return 0;
}
// ******************** End of 文件处理 ***************************************

// ******************** 系统时间与休眠 ****************************************
// 函数：获取系统日期时间信息。
// 输入：INT format_:
//		  format_== 0 返回日期时间格式：yyyy-mm-dd hh:mm:ss,例如：2008-03-12 21:12:23
//		  format == 1 返回日期时间格式：yyyy-mm-dd,例如：2008-03-12
//		  format == 2 返回日期时间格式：yyyy-mm-dd-hh:mm,例如：2008-03-12-12:28
//		  CHAR * sysTime_: 返回时间。
// 返回：时间字符串。
// 注释：基础函数。
char *knSysDateTime(INT format_, CHAR *myTime_) {
	time_t	rawTime;							// 系统时间变量。来源time.h定义。
#ifdef _WIN32
#ifdef KN_COMPLIE
	struct tm timeInfo;							// 系统时间数据静态结构。来源time.h定义
	time(&rawTime);								// 获取系统时间rawtime。
	// 将系统时间rawtime转变为本地时间并存放到时间结构timeinfo中。
	localtime_s(&timeInfo, &rawTime);			// localtime_s() Windows安全算法。
	timeInfo.tm_year = timeInfo.tm_year + 1900;	// 调整年。
	timeInfo.tm_mon = timeInfo.tm_mon + 1;		// 调整月。
	if(format_ == 0) {
		knSprintf(myTime_, "%4d-%02d-%02d %02d:%02d:%02d",
			timeInfo.tm_year,
			timeInfo.tm_mon,
			timeInfo.tm_mday,
			timeInfo.tm_hour,
			timeInfo.tm_min,
			timeInfo.tm_sec);
	}
	else if(format_ == 1) {
		knSprintf(myTime_, "%4d-%02d-%02d",
			timeInfo.tm_year,
			timeInfo.tm_mon,
			timeInfo.tm_mday);
	}
	else {
		knSprintf(myTime_, "%4d-%02d-%02d %02d:%02d",
			timeInfo.tm_year,
			timeInfo.tm_mon,
			timeInfo.tm_mday,
			timeInfo.tm_hour,
			timeInfo.tm_min);
	}
#else		// Windows非安全模式,ANSI C模式。
	time_t	rawTime;								// 系统时间变量。来源time.h定义。
	struct tm *timeInfo = NULL;						// 系统时间数据指针结构。来源time.h定义
	time(&rawTime);									// 获取系统时间rawtime。
	timeInfo = localtime(&rawTime);					// localtime() ANSI Ｃ算法。
	timeInfo->tm_year = timeInfo->tm_year + 1900;	// 调整年。
	timeInfo->tm_mon = timeInfo->tm_mon + 1;		// 调整月。
	if(format_ == 0) {
		knSprintf(myTime_, "%4d-%02d-%02d %02d:%02d:%02d",
			timeInfo->tm_year,
			timeInfo->tm_mon,
			timeInfo->tm_mday,
			timeInfo->tm_hour,
			timeInfo->tm_min,
			timeInfo->tm_sec);
	}
	else if(format_ == 1) {
		knSprintf(myTime_, "%4d-%02d-%02d",
			timeInfo->tm_year,
			timeInfo->tm_mon,
			timeInfo->tm_mday);
	}
	else {
		knSprintf(myTime_, "%4d-%02d-%02d %02d:%02d",
			timeInfo->tm_year,
			timeInfo->tm_mon,
			timeInfo->tm_mday,
			timeInfo->tm_hour,
			timeInfo->tm_min);
	}
#endif
#endif
#ifdef _LINUX32										// Linux编译环境, ANSI C模式。
    struct tm *timeInfo = NULL;						// 系统时间数据指针结构。来源time.h定义
    time(&rawTime);									// 获取系统时间rawtime。
    timeInfo = localtime(&rawTime);					// localtime() ANSI Ｃ算法。
    timeInfo->tm_year = timeInfo->tm_year + 1900;	// 调整年。
    timeInfo->tm_mon = timeInfo->tm_mon + 1;		// 调整月。
    if(NULL == myTime_) {
        knError("%s line %d warning: myTime_ is NULL.\n", __FILE__, __LINE__);
        return NULL;
    }
    if (format_ == 0) {
        knSprintf(myTime_, "%4d-%02d-%02d %02d:%02d:%02d",
                  timeInfo->tm_year,
                  timeInfo->tm_mon,
                  timeInfo->tm_mday,
                  timeInfo->tm_hour,
                  timeInfo->tm_min,
                  timeInfo->tm_sec);
    } else if (format_ == 1) {
        knSprintf(myTime_, "%4d-%02d-%02d",
                  timeInfo->tm_year,
                  timeInfo->tm_mon,
                  timeInfo->tm_mday);
    } else {
        knSprintf(myTime_, "%4d-%02d-%02d %02d:%02d",
                  timeInfo->tm_year,
                  timeInfo->tm_mon,
                  timeInfo->tm_mday,
                  timeInfo->tm_hour,
                  timeInfo->tm_min);
    }
#endif
	return myTime_;
}

// 杂项：秒休眠。
// 输入：INT seconds_ 秒。
// 返回：无。
// 注释：基础封装函数。
// 测试：测试通过。lxq20090929
void knSleep(INT seconds_) {
#ifdef _WIN32
	if(seconds_ == KN_WAIT_FOREVER) {
		Sleep((DWORD)seconds_);     // 单位：毫秒。
	}
	else {
		Sleep((DWORD)seconds_*1000);
	}
#endif
#ifdef _LINUX32
    if (seconds_ == KN_WAIT_FOREVER) {
        sleep((ULONG)seconds_); // 单位：秒。
    } else {
        sleep((ULONG)seconds_);
    }
#endif
}

// 杂项：毫秒休眠。
// 输入：INT milliseconds_毫秒,如果超时定义KN_WAIT_FOREVER,则无限等待。
// 返回：无。
// 注释：基础函数。
void knSleepM(INT milliseconds_) {
#ifdef _WIN32
		Sleep((DWORD)milliseconds_);
#endif
#ifdef _LINUX32
    if (milliseconds_ == KN_WAIT_FOREVER) {
        usleep((useconds_t )milliseconds_); //usleep 以微秒为基本单位
    } else {
        usleep((useconds_t )milliseconds_*1000);  //linux us * 1000 = ms
    }
#endif
}

// 函数：时刻。
// 输入：无。
// 返回：-1 失败。
// 注释：基础函数，获取当前时刻(秒计时时刻)。
DOUBLE knTime(void) {
	time_t timer;
	LONG ret;
	ret = (LONG)time(&timer);
	if(ret < 0) {
		return -1;
	}
	else {
		return ret;
	}
}

// 函数：时间间隔（秒间隔）。
// 输入：DOUBLE end_ 结束时间点, DOUBLE begin_ 开始时间点.
// 返回：时间间隔；-1 失败。
// 注释：本函数与knTime()结合使用。精度：精度只有秒级。
DOUBLE knDiffTime(DOUBLE end_, DOUBLE begin_) {
	DOUBLE ret;
	ret = difftime((time_t)end_, (time_t)begin_);
	if(ret < 0) {
		return -1;
	}
	else {
		return ret;
	}
}

// 函数：时钟计时时刻。
// 输入：无。
// 返回：-1 失败。
// 注释：获取当前时刻时钟计时时刻。
//       clock tick：时钟计时单元（而不把它叫做时钟滴答次数），一个时钟计时单元的时间
//		 长短是由CPU控制的。一个clock tick不是CPU的一个时钟周期，而是C/C++的一个基本计时单位。
LONG knClock(void) {
	return clock();
}

// 函数：时间间隔。
// 输入：LONG end_ 结束时间点, LONG begin_ 开始时间点.
// 返回：时间间隔；-1 失败。
// 注释：本函数与knClock()结合使用。精度：精度豪秒级。
DOUBLE knDiffColck(LONG end_, LONG begin_) {
	return (DOUBLE)(end_ - begin_);
}
// ******************** End of 系统时间与休眠 *********************************

// ******************** 磁盘文件操作 ******************************************
// 函数：封装fopen() 。
// 输入：const CHAR *filename_ 文件, const CHAR *mode_ 文件打开模式。
// 返回：文件句柄,NULL 失败。
// 注释：基础函数。本函数形参是fopen()的形参格式。
// 测试：测试通过。lxq20091026
void *knFopen(const CHAR *filename_, const CHAR *mode_) {
	void *st;
	if(NULL == (st = fopen(filename_, mode_))) {
		knError("%s Line %d warning: fopen() failed.\n", __FILE__, __LINE__);
	}
	return st;
}

// 函数：封装fread().读取文件.
// 函数：封装fread().读取文件.
// 输入：void *buf_ 读写到内存的内存文件指针, INT size_ 文件内存结构字节数,
//		 INT itemCount_ 文件行数(或记录数), void *file_ 磁盘文件句柄.
// 返回：文件item个数(例如,记录数)；-1失败.
// 注释：基础函数。本函数对fread()做了改进,可以从返回的buf_中直接读取从文件句柄中获取的数据.
INT knFread(void *buf_, INT size_, INT itemCount_, void *file_) {
    INT ret;
    if(NULL == file_) {
        knError("%s Line %d warnning: Input file_ is NULL.\n", __FILE__, __LINE__);
        return -1;
    }
    // fread is ANSI C.
    ret = (INT)fread(buf_, size_, itemCount_, (FILE*)file_);
	if(ret > 0) {
		return ret;
	}
	else {
		return -1;
	}
}

// 函数：封装fwrite().将内存文件写入磁盘文件.
// 输入：const void *buf_ 准备存盘的内存文件指针, INT size_ 文件内存结构字节数,
//		 INT count_ 文件行数(或记录数), void *file_ 磁盘文件句柄.
// 返回：文件item个数(例如,记录数)；-1 失败.
// 注释：基础函数。
INT knFwrite(const void *buf_, INT size_, INT count_, void *file_) {
    INT ret;
    if(NULL == file_) {
        knError("%s Line %d warnning: Input file_ is NULL.\n", __FILE__, __LINE__);
        return -1;
    }
    // fwrite is ANSI C.
    ret = (INT)fwrite(buf_, size_,count_, (FILE*)file_);
	if(ret > 0) {
		return ret;
	}
	else {
		return -1;
	}
}

// 函数：封装fclose()。关闭磁盘文件。
// 输入：void *file_ 文件句柄。
// 返回：0 成功；-1失败。
// 注释：基础函数。
INT knFclose(void *file_) {
    if(NULL == file_) {
        knError("%s Line %d warnning: Input file_ is NULL.\n", __FILE__, __LINE__);
        return -1;
    }
    // fclose() is ANSI C.
	if(0 == fclose((FILE*)file_)) {
		return 0;
	}
	else {
		return -1;
	}
}

// 函数：读磁盘文件。
// 输入：char *file_ 文件名, knFileCB fcb_ 行内容处理回调函数,
//       void *result_ 输出结果，一般情况下，result_是一个链表。
// 返回：0 成功；-1失败。
// 注释：读磁盘文件的内容，每行内容由fcb_处理。
int knReadFile(const char *file_, knFileCB fcb_, void *result_) {
	FILE *fp;
	int i = 0;
	char buf[KN_LINE_MAXBYTES];
	if(NULL == (fp = (FILE*)knFopen(file_, "r+"))) {
		knError("%s Line %d warning: knFopen() failed.\n", __FILE__, __LINE__);
		return -1;
	}
	while(!feof(fp)) {	// 遍历文件内含行记录。
		knBzero(buf, KN_LINE_MAXBYTES);
		if(NULL != fgets(buf, KN_LINE_MAXBYTES, fp)) {
			i++;
			if(-1 == fcb_(buf, result_)) {
				knError("%s Line %d warning: fcb_() failed.\n", __FILE__, __LINE__);
			}
		}
	}
	fclose(fp);
	return i;
}
// ******************** End of 磁盘文件操作 ***********************************

// ******************** 获取目录下的文件 **************************************
// 本函数在knDataUtil.c文件中实现。
// ******************** End of 获取目录下的文件 *******************************

// ******************** 单双字节转换 ******************************************
// 函数：将单字符字符串转换为双字节字符串。
// 输入：CHAR *sbcs_ 单字节字符串(in), INT sbcsSize_ 单字节字符串字节数(in),
//		 CHAR *dbcs_ 双字节字符串(out), INT dbcsSize_ 双字节字符串字节数(in)。
// 返回：双字节字符串；NULL失败.
// 注释：基础函数。
//		 single-byte character set (SBCS) pages or double-byte character set (DBCS)
//		 size_ 用于告知knSBCS2DBCS()双字节字符串允许最大字节数,一般用静态变量定义dbcs_。
//		 Maps a character string to a UTF-16 (wide character) string. The character
//		 string is not necessarily from a multibyte character set.
//		 Caution  Using the MultiByteToWideChar function incorrectly can compromise the
//		 security of your application. Calling this function can easily cause a buffer
//		 overrun because the size of the input buffer indicated by lpMultiByteStr equals
//		 the number of bytes in the string, while the size of the output buffer indicated
//		 by lpWideCharStr equals the number of characters. To avoid a buffer overrun, your
//		 application must specify a buffer size appropriate for the data type the buffer
//		 receives. For more information, see Security Considerations: International Features.
//		 Java或C#接口时，使用返回。
CHAR *knSBCS2DBCS(CHAR *sbcs_, INT sbcsSize_, CHAR *dbcs_, INT dbcsSize_) {
#ifdef _WIN32
	if(NULL == sbcs_) {
		knError("%s Line %d warnning: Input sbcs_ is NULL.\n", __FILE__, __LINE__);
		return NULL;
	}
	if(0 == MultiByteToWideChar(CP_ACP,
								MB_COMPOSITE,
								sbcs_,
								sbcsSize_,
								(LPWSTR)dbcs_,
								dbcsSize_)) {
		knError("%s Line %d warnning:  MultiByteToWideChar() failed by err %d.\n", __FILE__, __LINE__, GetLastError());
		return NULL;
	}
	return dbcs_;
#endif
#ifdef _LINUX32
    wchar_t * kn_dbcs_;
    kn_dbcs_ = (wchar_t*)dbcs_;
    if (NULL == sbcs_) {
        knError("%s Line %d warnning: Input sbcs_ is NULL.\n", __FILE__, __LINE__);
        return NULL;
    }
    //kn_dbcs_ = (wchar_t *)malloc(dbcsSize_);		// 本处有内存泄露嫌疑，没有释放本处申请的内存。？？？？？李小群，待完善。
    if (-1 == mbstowcs((wchar_t *)dbcs_, sbcs_, (size_t)dbcsSize_)) {
        knError("%s Line %d warnning:  mbstowcs() failed.\n", __FILE__, __LINE__);
        return NULL;
    }
    return dbcs_;
#endif
}

// 函数：将双字符字符串转换为单字节字符串。
// 输入：CHAR *dbcs_ 双字节字符串(in), INT dbcsSize_ 双字节字符串字节数(in),
//		 CHAR *sbcs_ 单字节字符串(out), INT sbcsSize_ 单字节字符串字节数(in)。
// 返回：双字节字符串；NULL失败.
// 注释：single-byte character set (SBCS) pages or double-byte character set (DBCS)
//		 Java或C#接口时，使用返回。
CHAR *knDBCS2SBCS(CHAR *dbcs_, INT dbcsSize_, CHAR *sbcs_, INT sbcsSize_) {
#ifdef _WIN32
	INT err;
	if(NULL == dbcs_) {
		knError("%s Line %d warnning: Input dbcs_ is NULL.\n", __FILE__, __LINE__);
		return NULL;
	}
	// 没有完全掌握WideCharToMultiByte()参数的正确设置.lxq20100621
	if(0 == WideCharToMultiByte(CP_ACP,
                                WC_NO_BEST_FIT_CHARS,
                                (LPCWSTR)dbcs_,
                                dbcsSize_,
                                (LPSTR)sbcs_,
                                sbcsSize_,
                                NULL,
                                NULL)) {
        err = GetLastError();
        if(ERROR_INSUFFICIENT_BUFFER == err) {
            //knError("knBaseUtil::knDBCST2SBCS() warnning:  WideCharToMultiByte(INSUFFICIENT_BUFFER) failed by err %d\n", err);
            return sbcs_;		// 虽然出现本类错误,但是,输出是正确的。
		}
		else if(ERROR_INVALID_FLAGS == err) {
			knError("%s Line %d warnning:  WideCharToMultiByte(INVALID_FLAGS) failed by err %d\n", __FILE__, __LINE__, err);
			return NULL;
		}
		else if(ERROR_INVALID_PARAMETER == err) {
			knError("%s Line %d warnning:  WideCharToMultiByte(INVALID_PARAMETER) failed by err %d\n", __FILE__, __LINE__, err);
			return NULL;
		}
		else {
			return NULL;
		}
	}
	return sbcs_;
#endif
#ifdef _LINUX32
    if (NULL == dbcs_) {
        knError("%s Line %d warnning: Input dbcs_ is NULL.\n", __FILE__, __LINE__);
        return NULL;
    }
    if (-1 == wcstombs(sbcs_, (wchar_t*)dbcs_, (size_t)sbcsSize_)) {
        knError("%s Line %d warning: wcstombs() failed\n", __FILE__, __LINE__);
        return NULL;
    }
    return sbcs_;
#endif
}
// ******************** End of 单双字节转换 ***********************************

// ******************** 字符串操作函数封装 ************************************
/** 说明：Windows 2005声明：ANSI函数 strcpy()；strtok()；sprintf();fopen()；
		  vsprintf()
		  有安全隐患,并做了安全升级,针对Windows 2005的升级,特对上述函数封装,保
		  证 Windows 2005以上环境下编译与标准ANSI环境编译兼容.
*/

// 函数：封装strcpy() 。
// 输入：CHAR *dest_ 目标字符串, const CHAR *source_ 源字符串。
// 返回：复制字符串。
// 注释：基础函数。本函数形参是strcpy()的形参格式。
// 测试：通过。lxq20091026
CHAR *knStrcpy(CHAR *dest_, const CHAR *source_) {
#ifdef _WIN32
#ifdef KN_COMPLIE
	// 用源字符串长度作为目标字符串长度的占用字节数。
	strcpy_s(dest_, (strlen(source_) + 1), source_);
#else
	strcpy(dest_, source_);
#endif
#endif
#ifdef _LINUX32
    strcpy(dest_, source_);
#endif
	return dest_;
}

// 函数：封装strcat() 。
// 输入：CHAR *dest_ 目标字符串, INT len_, 源字符总长度, const CHAR *source_ 源字符串。
// 返回：复制字符串。
// 备注：在Linux模式下,len_参数为0。
CHAR *knStrcat(CHAR *dest_, INT len_, const CHAR *source_) {
#ifdef _WIN32
#ifdef KN_COMPLIE
    // 用源字符串长度作为目标字符串长度的占用字节数。
    strcat_s(dest_, len_ + 1, source_);
#else
    strcat(dest_, source_);
#endif
#endif
#ifdef _LINUX32
    strcat(dest_, source_);
#endif
	return dest_;
}

// 函数：封装strcmp() .
// 输入：CHAR *str1_ 第一个比较字符串, onst CHAR *str2_ 第二个比较字符串.
// 返回：0 字符串相同，-1 字符串不同。
// 注释：
INT knStrcmp(CHAR *str1_, const CHAR *str2_) {
	if(0 == strcmp(str1_, str2_)) {
		return 0;
	}
	else {
		return -1;
	}
}

// 函数：封装strlen() .
// 输入：CHAR *str_ 符串.
// 返回：字符串长度；-1 失败。
// 注释：
INT knStrlen(CHAR *str_) {
	if(NULL != str_)
		return strlen(str_);
	else
		return -1;
}

// 函数：封装strtok() 。
// 输入：CHAR *token_ 字符串(in_out), const CHAR *delimit_ 分隔符,
//		  CHAR **context_ 两次回调之间存储信息(只用于Windows 2005编译环境)。
// 返回：指向下一子串, NULL 未截出无子串了。
// 注释：基础函数。本函数形参是strtok()的形参格式。
// 注释：如果截取到子串,并且分隔符在原字符串首位,则token_返回原先的字符串,返回是原
//		 字符串被截取后的剩余子串；如果分隔符不在原字符串首位,则token_与返回均返回截
//		 取的分隔符前面子串(token_与返回相同)；如果没有截取到子串,则token_返回原字符
//		 串,返回为NULL。使用本函数时请一定注意上述情况。
//		 形参context_在调用本函数前定义变量是CHAR *context,实参格式&context。
// 测试：测试通过。lxq20091026
CHAR *knStrtok(CHAR *token_, const CHAR *delimit_, CHAR **context_) {
	CHAR *prev = NULL;		// 分割符截前的子串。
#ifdef _WIN32
	size_t len;				// 字符串长度。
	if(NULL == token_) {
		len = 0;
	}
	else {
		len = strlen(token_);	// 定义字符串长度。
	}
#ifdef KN_COMPLIE
	// token_是待判断的字符串,delimit_是字符串内要判断的分隔符,strtok_s（）
	// 如果检测到在token_里面有分隔符delimit_,则在token_与prev中返回分隔符
	// 前的子串(token_ == prev),next返回分隔符后的子串。如果没有检测到分隔符,
	// 则token_与prev返回长度为0的空串。
	prev = strtok_s(token_, delimit_, context_);
#else
	prev = strtok(token_, delimit_);
#endif
	if(0 != len) {
		if(len == strlen(token_)) {	// 如果没有截到有效子串。
			return NULL;
		}
		else {						// 截到有效子串。
			return prev;
		}
	}
	else {
		return prev;
	}
#endif
#ifdef _LINUX32
    size_t len;
    if (NULL == token_) {
        len = 0;
    } else {
        len = strlen(token_);
    }
    prev = strtok(token_, delimit_);
    if (0 != len) {
        if (len == strlen(token_)) {
            return NULL;
        } else {
            return prev;
        }
    } else {
        return prev;
    }
#endif
}

// 函数：封装sprintf() 。
// 输入：const CHAR *fmt_ 输入字符串；.....。
// 返回：复制字符串字符数；-1 失败。
// 注释：本函数形参是sprintf()的形参格式。
// 测试：测试通过。lxq20091026
INT knSprintf(CHAR *buf_, const CHAR *fmt_, ...) {
	INT ret = 0;
	va_list	ap;
	va_start(ap, fmt_);
	ret = knVsprintf(buf_, fmt_, ap);
	va_end(ap);
	return ret;
}

// 函数：封装vsprintf() 。
// 输入：CHAR *buffer_ 输出内存, const CHAR *format_ 格式定义, CHAR *(=va_list) argptr_ 变量列表指针。
// 返回：复制字符串字符数；-1 失败。
// 注释：基础函数。本函数形参是vsprintf()的形参格式。
// 测试：测试通过。lxq20091026
INT knVsprintf(CHAR *buffer_, const CHAR *format_, CHAR *argptr_) {
#ifdef _WIN32
#ifdef KN_COMPLIE
	INT len;
	len = _vscprintf(format_, argptr_) + 1;
	if(PRINT_INPUTMAXBYTES < len) {
		knError("%s Line %d warning: len(%d bytes) of context can not be more thand %d bytes.\n", __FILE__, __LINE__, len, PRINT_INPUTMAXBYTES);
		return -1;
	}
	return vsprintf_s(buffer_, len, format_, argptr_);
#else
	return vsprintf(buffer_, format_, argptr_);
#endif
#endif
#ifdef _LINUX32
    return vsprintf(buffer_, format_, argptr_);
#endif
}

// 函数：从一个字符串中查找是否有子串.
// 输入：CHAR *str_ 目标字符串, CHAR *subStr_ 子字符串.
// 返回：子字符串在目标字符串中的起始位置；-1 没有找到.
// 注释：要求子串必须在目标字符串中连续发现，不允许中间
//		 出现断裂。
INT knCheckStringFromString(CHAR *str_, CHAR *subStr_) {
	int i;			// 目标数据遍历计数器。
	int j;			// 子串数据遍历计数器。
	int len1;		// 目标数据字节数。
	int len2;		// 子串数据字节数。
	int k = 0;		// 子串在目标字符串中匹配计数器。
	if(NULL == str_ || NULL == subStr_) {
		knError("knBaseUtil:: knGetStringFromString() warning: str_ or subStr_ may be NULL.\n", __FILE__, __LINE__);
		return -1;
	}
	len1 = strlen(str_);
	len2 = strlen(subStr_);
	if(len2 > len1) {
		knError("knBaseUtil:: knGetStringFromString() warning: The length of subStr is more than mainStr.\n", __FILE__, __LINE__);
		return -1;
	}
	for(i = 0; i < len1; i++) {
		for(j = 0; j < len2; j++) {
			if(str_[i] == subStr_[j]) {
				i++;
				k++;
			}
		}
		if(len2 == k) {
			return i - len2 + 1;
		}
	}
	return -1;
}

// 函数：删除输入字符串的符号前的字串前缀.
// 输入：CHAR *buffer_ 输入字符串(in_out), CHAR symbol_ 前缀符号.
// 返回：处理后的字符串。
CHAR *knCutPrefix(CHAR *buffer_, CHAR symbol_) {
	CHAR temp[PRINT_INPUTMAXBYTES];
	INT i, j = 0;
	INT len;
	INT q = 0;		// 处理字符串标识：q = 0 不复制；q = 1 复制。
	memset(temp, 0, PRINT_INPUTMAXBYTES);
	len = (INT)strlen(buffer_);
	if(len > PRINT_INPUTMAXBYTES) {
		memcpy(temp, buffer_, PRINT_INPUTMAXBYTES);
	}
	else {
		knSprintf(temp, buffer_);
	}
	memset(buffer_, 0, len);
	for(i = j = 0; i < len; i++) {
		if(symbol_ == temp[i] && 0 == q) {
			q = 1;
			continue;
		}
		if(1 == q) {
			buffer_[j] = temp[i];
			j++;
		}
	}
	return buffer_;
}

// 函数：删除输入字符串的符号后的字符串后缀。
// 输入：CHAR *buffer_ 输入字符串(in_out), CHAR symbol_ 后缀分隔符号。
// 返回：处理后的字符串。
CHAR *knCutPostfix(CHAR *buffer_, CHAR symbol_) {
	CHAR temp[KN_LINE_MAXBYTES];
	INT i, j;
	INT len;
	memset(temp, 0, KN_LINE_MAXBYTES);
	len = (INT)strlen(buffer_);
	if(len > KN_LINE_MAXBYTES) {
		memcpy(temp, buffer_, KN_LINE_MAXBYTES);
	}
	else {
		knSprintf(temp, buffer_);
	}
	memset(buffer_, 0, len);
	for(i = j = 0; i < len; i++, j++) {
		if(symbol_ == temp[i]) {
			break;
		}
		else {
			buffer_[j] = temp[i];
		}
	}
	return buffer_;
}

// 函数：过滤字符处理。内含字符串去空格,Tab空格等优化处理。
// 输入：CHAR *str_: 输入字符串指针(in_out)；CHAR filter_: 过滤字符。
// 返回：处理后的字符串。
CHAR *knFilterChar(CHAR *str_, CHAR filter_) {
	INT	i = 0;			// 源字符串操作计数器。
	INT j = 0;			// 目标字符串操作计数器。
	INT len;			// 字符串长度。
	CHAR dstr[KN_LINE_MAXBYTES];		// 目标变量名字符串。
	memset(dstr, 0, KN_LINE_MAXBYTES);
	len = (INT)strlen(str_);
	for(i = 0; i < len; i++) {
		if(str_[i] == '/') { // 处理反斜杠"/"注释,两个反斜杠后的内容被抛弃。
			if(str_[i+1] == '/')
				break;
		}
		// 过滤抽取字符(filter_)、空格、Tab空格、断行符。
		if(str_[i] == filter_ || str_[i] == ' ' || str_[i] == '\t' || str_[i] == '\n') {
			continue;
		}
		dstr[j] = str_[i];	// 变量名转移操作。
		j++;
	}
	dstr[j+1] = '\0';
	knStrcpy(str_, dstr);
	return str_;
}

// 函数：过滤字符处理.内含字符串去空格,Tab空格等优化处理.
// 输入：CHAR *str_: 输入字符串指针(in_out)；CHAR filter_: 过滤字符.
// 返回：处理后的字符串。
// 注释：默认过滤去除空格、Tab空格, \t,\n,以及定义的过滤字符filter_，
//       不过滤“//”后的注释信息。
CHAR *knFilterChar2(CHAR *str_, CHAR filter_) {
	INT	i = 0;			// 源字符串操作计数器。
	INT j = 0;			// 目标字符串操作计数器。
	INT len;			// 字符串长度。
	CHAR dstr[KN_LINE_MAXBYTES];		// 目标变量名字符串。
	memset(dstr, 0, KN_LINE_MAXBYTES);
	len = (INT)strlen(str_);
	for(i = 0; i < len; i++) {
		// 过滤抽取字符(filter_)、空格、Tab空格、断行符。
		if(str_[i] == filter_ || str_[i] == ' ' || str_[i] == '\t' || str_[i] == '\n') {
			continue;
		}
		dstr[j] = str_[i];	// 变量名转移操作。
		j++;
	}
	dstr[j+1] = '\0';
	knStrcpy(str_, dstr);
	return str_;
}

// 函数：删除输入字符串的前后或之间的空格,或Tab空格.
// 输入：CHAR *str_ 字符串指针(in_out).
// 返回: 无.
// 注释：由于本函数调用knFilterChar()实现本功能，因此，能够将字符串之间的空格全部删除。
CHAR *knTrim(CHAR *str_) {
	knFilterChar(str_, ' ');
	knStrcpy(str_, str_);
	return str_;
}

// 函数：只删除输入字符串的前后空格,或Tab空格，不删除字符串内部之间空格.
// 输入：CHAR *str_ 字符串指针(in_out).
// 返回：无.
CHAR * knCutStrSpace(CHAR *str_) {
	INT len;							// 字符串长度。
	INT i, j = 0;						// 计数器。
	CHAR tmp[KN_LINE_MAXBYTES];			// 中间变量字符串。
	if(NULL == str_) {
		knError("%s Line %d warning: str_ is NULL.\n", __FILE__, __LINE__);
		return NULL;
	}
	len = knStrlen(str_);
	knBzero(tmp, KN_LINE_MAXBYTES);
	// 清除字符串前缀空格。
	for(i = 0; i < len; i++) {
		if((' ' == str_[i]) || ('\t' == str_[i])) {
			continue;
		}
		else {
			memcpy(tmp, &str_[i], (len - i));
			knBzero(str_, len);
			memcpy(str_, tmp, (len - i));
			break;
		}
	}
	// 清除字符串后缀空格。
	len = knStrlen(str_);
	knBzero(tmp, KN_LINE_MAXBYTES);
	for(i = len-1; i > 0; i--) {
		if((' ' == str_[i]) || ('\t' == str_[i])) {
			j++;
			continue;
		}
		else {
			memcpy(tmp, &str_[0], (len - j));
			knBzero(str_, len);
			memcpy(str_, tmp, (len - j));
			break;
		}
	}
	return str_;
}

// 函数：处理一个不规范的字符串，使之每个词之间有一个空格。
// 输入：CHAR *str_: 输入字符串指针(in_out)。
// 返回：无。
CHAR *knOptimizeString(CHAR *str_) {
	INT	i = 0;	        // 源字符串操作计数器。
	INT j = 0;			// 目标字符串操作计数器。
	INT len;              // 字符串长度。
	INT q = 1;			// 两个字符或字符串之间只有最后一个空格、或tab时，q = 0，否则q = 1。
	INT f = 0;            // f = 0 首字符为空; f = 1 有字符。
	CHAR dstr[KN_LINE_MAXBYTES];		// 目标变量名字符串。
	memset(dstr, 0, KN_LINE_MAXBYTES);
	len = (INT)strlen(str_);
	for(i = 0; i < len; i++) {
		// 过滤抽取空格、Tab空格、断行符。
		if(str_[i] == '\n') {
			continue;
		}
		else if(str_[i] == ' ' || str_[i] == '\t') {
			q = 0;
			continue;
		}
		else {
			if(0 == q) {
			    if(1 == f) {
                    dstr[j] = ' ';
                    j++;
			    }
			}
			dstr[j] = str_[i];	// 变量名转移操作。
			j++;
			q = 1;
			f = 1;
		}
	}
	dstr[j+1] = '\0';
	knStrcpy(str_, dstr);
	return str_;
}

// 函数：处理半个汉字问题。
// 输入：string &s 字符串
// 返回：输出字节数。
// 注释：如果输入字符串内有唯一一个半个汉字，则用本方法处理，副作用是将半个汉字后的字符串全部删除，本函数一般用于处理字符串末尾的半个汉字。
CHAR *knDeleteLastChar(CHAR *s_) {
	INT count = 0;				// 半个汉字的位置。
	INT i;						// 计数器。
	INT len = strlen(s_);
	CHAR *buf;
	if(NULL == (buf = (CHAR*)malloc(len))) {
		knError("%s Line %d warning: malloc() failed.\n", __FILE__, __LINE__);
		return NULL;
	}
	knBzero(buf, len);
	memcpy(buf, s_, len);
	knBzero(s_, len);
	for(i = 0; i < len; i++) {
		if((buf[i] & 0x80) == 0x80) {
			count++;
		}
	}
	if(1 == count % 2) {
		memcpy(s_, buf, (len - count));
	}
	free(buf);
	return s_;
}

// 函数：从字符串中获取Key Value。.
// 输入：CHAR *key_ 获取Key(in_out), CHAR *value_ 获取Value(in_out),
//       CHAR symbol_ Key Value分隔符(in), const CHAR *str_ 源字符串(in),
//	     INT mode_ 处理字符串模式：0 过滤空格等；1 不过滤。
// 返回：0 成功；-1 失败
INT knGetKeyValueFromStr(CHAR *key_, CHAR *value_, CHAR symbol_, const CHAR *str_, INT mode_) {
	INT len;							// str_长度。
	INT i;								// str_移位计数器。
	INT j = 0;							// myKey移位计数器。
	INT k = 0;							// myValue移位计数器。
	INT f = 0;							// Key Value位置标识：f = 0 Key位置； f = 1 Value位置。
	INT q = 0;							// 首次识别分隔符：q = 0 未识别分隔符；q = 1 首次识别分隔符。
	CHAR myKey[64];		// Key字符串。
	CHAR myValue[KN_LINE_MAXBYTES];		// Value字符串。
	knBzero(myKey, sizeof(myKey));
	knBzero(myValue, KN_LINE_MAXBYTES);
	if(NULL == str_) {
		knError("%s Line %d warning: str_ is NULL.\n", __FILE__, __LINE__);
		return -1;
	}
	len = (INT)strlen(str_);
    for(i = 0; i < len; i++) {
		if(symbol_ == str_[i]) {
			f = 1;
			if(0 == q) {
				q = 1;
				continue;
			}
		}
		if(str_[i] == '\n') {
			continue;
		}
		if((KN_STR_FILTER == mode_) && (str_[i] == ' ' || str_[i] == '\t')) {
			continue;
		}
		if(0 == f) {
			if(63 < j) {
				knError("%s Line %d warning:: The length of myKey is more than 64.\n", __FILE__, __LINE__);
				return -1;
			}
			myKey[j] = str_[i];
			j++;
		}
		else if(1 == f) {
			if((KN_LINE_MAXBYTES-1) < k) {
				knError("%s Line %d warning:: The length of myVaulue is more than %d.\n", __FILE__, __LINE__, KN_LINE_MAXBYTES);
				return -1;
			}
			myValue[k] = str_[i];
			k++;
		}
	}
	if(0 != strlen(myKey)) {
		knStrcpy(key_, myKey);
	}
	else {
		return -1;
	}
	knStrcpy(value_, myValue);
	return 0;
}

// 函数：将字符串str_从第一个分割符处分割为两个字符串。
// 输入：CHAR *prevStr_ 获取分割符之前的内容
//       CHAR *afterStr_ 获取分割符之后的内容,
//       CHAR symbol_ 分隔符(in),
//       const CHAR *str_ 源字符串(in),
//		 INT mode_ 处理字符串模式：0 不过滤空格等；1 过滤空格。
// 返回：0 成功；-1 失败；1 结束处理。
INT knGetSubstringFromStr(CHAR *prevStr_, CHAR *afterStr_, CHAR symbol_, const CHAR *str_, INT mode_) {
	INT len;								// str_长度。
	INT i;									// str_移位计数器。
	INT j = 0;								// prevStr_移位计数器。
	CHAR myPrev[64];						// 解析出的字符串。
	knBzero(myPrev, 64);
	if(NULL == str_) {
		knError("%s Line %d warning: str_ is NULL.\n", __FILE__, __LINE__);
		return -1;
	}
	len = (INT)strlen(str_);
	for(i = 0; i < len; i++) {
		if((KN_STR_FILTER == mode_) && (' ' == str_[i] || '\t' == str_[i] || '\n' == str_[i])) {
			continue;
		}
		if(symbol_ == str_[i]) {
		    memcpy(afterStr_, str_+i+1, len-i-1);
		    if(KN_STR_FILTER == mode_) {
		        // 过滤空格。
		        knCutStrSpace(afterStr_);
		    }
		    break;
		}
		myPrev[j] = str_[i];
		j++;
	}
	if(0 != strlen(myPrev)) {
		knStrcpy(prevStr_, myPrev);
	}
	else {
		return 1;
	}
	return 0;
}

// 函数：截取位置前字符串。
// 输入：string &s 字符串(in_out), INT pos_子串截取位置。
// 返回：处理后的字符串。
// 注释：无。
CHAR *knCutSubStr(CHAR *s_, INT pos_) {
	char subStr[32];
	int i;
	knBzero(subStr, sizeof(subStr));
	for(i = 0; i < pos_; i++) {
		subStr[i] = s_[i];
	}
	knBzero(s_, knStrlen(s_));
	knStrcpy(s_, subStr);
	return s_;
}

// 函数：截取字符串标识符后的几位数.
// 输入：CHAR *str_ 字符串, char symb_ 分隔符, int pos_ 保留分隔符后的字符数量
// 返回：返回截取的子串.
// 注释：
CHAR *knCutStringPos(CHAR *str_, char symb_, int pos_) {
	char body[32];
	char end[16];
	knBzero(body, sizeof(body));
	knBzero(end, sizeof(end));
	knGetKeyValueFromStr(body, end, symb_, str_, 0);
	knCutSubStr(end, pos_);
	knBzero(str_, knStrlen(str_));
	knSprintf(str_, "%s.%s", body, end);
	return str_;
}

/** 函数：从后往前截取字符串。
输入：char *bgStr_ 截取前段子串, char *edStr_ 截取后段子串,  
      char *str_ 原始字符串, int len_ 原始字符串长度, char flag_ 分隔字符。
返回：0 成功；-1 失败。
注释：原始字符串默认最大长度len < KN_EXEC_PATH；
      
*/
int knCutStrFromEndToBegin(char *bgStr_, char *edStr_, char *str_, int len_, char flag_) {
	char tmp[64];
	int i, j;
	int ret = -1;
	knBzero(tmp, sizeof(tmp));
	for(i = len_, j = 0; i > 0; i--, j++) {
		tmp[i] = str_[i];
		if(str_[i] == flag_) {
			ret = 0;
			break;
		}
		if(64 <= j) {
			break;
		}
	}
	if( 0 == ret) {
		// 复制源字段前一段子串。
		if(NULL != bgStr_) {
			memcpy(bgStr_, str_, i+1);
		}
		// 复制源字段后一段子串。
		if(NULL != edStr_) {
			memcpy(edStr_, tmp + i + 1, j);
		}
	}
	return ret;
}

// 函数：小写字符串转大写字符串。
// 输入：CHAR *str_ 小写字符串.
// 返回：大写字符串；NULL 失败。
// 注释：
CHAR *knStr2Upper(CHAR *str_) {
    INT len;
	INT i;
	CHAR tmp[32];
	if(NULL == str_) {
		knError("%s Line %d warnning: str_ is NULL.\n", __FILE__, __LINE__);
		return NULL;
	}
	knBzero(tmp, sizeof(tmp));
	len = knStrlen(str_);
    for(i = 0; i < len; i++) {
        tmp[i] = (CHAR)toupper(str_[i]);
	}
	knStrcpy(str_, tmp);
	return str_;
}

// 函数：大写字符串转小写字符串。
// 输入：CHAR *str_ 大写字符串.
// 返回：小写字符串；NULL 失败。
// 注释：
CHAR *knStr2Lower(CHAR *str_) {
    INT len;
	INT i;
	CHAR tmp[32];
	if(NULL == str_) {
		knError("%s Line %d warnning: str_ is NULL.\n", __FILE__, __LINE__);
		return NULL;
	}
	knBzero(tmp, sizeof(tmp));
	len = knStrlen(str_);
    for(i = 0; i < len; i++) {
        tmp[i] = (CHAR)tolower(str_[i]);
	}
	knStrcpy(str_, tmp);
	return str_;
}

/** 函数：用新符号替换字符串中的旧符号。
输入：char *str_ 字符串(in_out), int len_ 字符串长度, char before_ 原有的符号,
      char after_ 替换的符号。
返回：输出字符串；-1 失败。
注释：在输入字串串中，有特定的符号需要用新符号替换。
*/
INT knReplaceSymbol(CHAR *str_, INT len_, CHAR before_, CHAR after_) {
	int i;
	if(NULL == str_) {
		knError("%s Line %d warning: str_ is NULL.\n", __FILE__, __LINE__);
		return -1;
	}
	for(i = 0; i < len_; i++) {
		if(before_ == str_[i]) {
			str_[i] = after_;
		}
	}
	return i;
}
// ******************** End of 字符串操作函数封装 *****************************

// ******************** 数字字符串转换 ****************************************
// 函数：将十进制整形数转变为字符串。
// 输入：INT in_ 输入整形数(in), CHAR *out_ 输出字符串(in_out)。
// 返回：0 成功；-1 失败。
CHAR *knItoa(INT in_, CHAR *out_) {
	knSprintf(out_, "%d", in_);
	return out_;
}

// 函数：将十进制长整形数转变为字符串。
// 输入：LONG in_ 输入长整形数(in), CHAR *out_ 输出字符串(in_out)。
// 返回：0 成功；-1 失败。
CHAR *knLtoa(LONG in_, CHAR *out_) {
	knSprintf(out_, "%d", in_);
	return out_;
}

// 函数：将浮点数转变为字符串函数.
// 输入：FLOAT in_ 输入整形数(in), INT p_ 保留小数点，CHAR *out_ 输出字符串(in_out).
// 返回：字符串.
// 返回：整形数；-1 失败.
// 注释：默认输入in_字符为10进制数字字符串,CHAR *out_字节长度为32字节.
CHAR *knFtoa(FLOAT in_, INT p_, CHAR *out_) {
	knSprintf(out_, "%f", in_);
	knCutStringPos(out_, '.', p_);
	return out_;
}

// 函数：将双精浮点数转变为字符串函数.
// 输入：DOUBLE in_ 输入双精浮点数(in), INT p_ 保留小数点，CHAR *out_ 输出字符串(in_out).
// 返回：字符串.
// 返回：整形数；-1 失败.
// 注释：默认输入in_字符为10进制数字字符串,CHAR *out_字节长度为32字节.
CHAR *knDtoa(DOUBLE in_, INT p_, CHAR *out_) {
	knBzero(out_, sizeof(out_));
	knSprintf(out_, "%f", in_);
	knCutStringPos(out_, '.', p_);
	return out_;
}

// 函数：将字符串整形数转变为INT型整形数。
// 输入：CHAR *in_ 输入字符串,INT *out_ 输出整形数指针。
// 返回：0 成功；-1 失败。
INT knAtoi(CHAR *in_, INT *out_) {
	int ret = 0;
	if(0 == knStrcmp(in_, "0")) {
		*out_ = 0;
	}
	else {
		if(0 == (ret = atoi(in_))) {
			knError("knBaseUtil::knAtoi() warnning: atoi() failed.\n", __FILE__, __LINE__);
			return -1;
		}
	}
	if(NULL != out_) {
		*out_ = ret;
	}
	return ret;
}

// 函数：将字符串整形数转变为LONG型整形数。
// 输入：CHAR *in_ 输入字符串,LONG *out_ 输出整形数指针。
// 返回：0 成功；-1 失败。
LONG knAtol(CHAR *in_, LONG *out_) {
	long out;
	if(0 == knStrcmp(in_, "0")) {
		out = 0;
	}
	else {
		if(0 == (out = atol(in_))) {
			knError("%s Line %d warnning: atol() failed.\n", __FILE__, __LINE__);
			return -1;
		}
	}
	if(NULL != out_) {
		*out_ = out;
	}
	return out;
}

// 函数：将字符串浮点数转变为FLOAT型数函数. ok
// 输入：CHAR *in_ 输入字符串, FLOAT *out_ 输出整形数指针.
// 返回：长整形数；-1 失败.
// 注释：默认输入in_字符为10进制数字字符串.
FLOAT knAtof(CHAR *in_, FLOAT *out_) {
	float ret;
    //ret = strtod(in_, NULL);
	ret = (float)atof(in_);
	if(NULL != out_) {
		*out_ = ret;
	}
    return ret;
}

// 函数：将字符串双精浮点数转变为DOUBLE型数函数.
// 输入：CHAR *in_ 输入字符串, DOUBLE *out_ 输出整形数指针.
// 返回：长整形数；-1 失败.
// 注释：默认输入in_字符为10进制数字字符串.
DOUBLE knAtod(CHAR *in_, DOUBLE *out_) {
    double ret;
	ret = strtod(in_, NULL);
	if(NULL != out_) {
		*out_ = ret;
	}
    return ret;
}

// 函数：封装四舍五入函数。
// 输入：DOUBLE in_ 需要四舍五入的浮点数, INT decimal_ 保留有效小数点位数。
// 返回：四舍五入结果；-1 失败。
// 注释：返回四舍五入的结果浮点数。
// 注释：本函数的算法没有完全搞懂,但是,运算执行结果正确。
DOUBLE knRound(DOUBLE in_, INT decimal_) {
	// 说明：该函数只关心保留小数位数的后一位,与sprintf函数相同
	// 例：knRound(0.499,2)=0.50； knRound(0.4949,2)=0.49
	double num, dec, factor;
	dec  = modf(in_, &num);				// 分切整数与小数部分(不直接用long型数参与计算,避免运算溢出)
	factor = pow((double)10, decimal_);	// 因子
	dec *= factor;						// 小数点左移decimalnum位
	dec += in_ >= 0 ? 0.5: -0.5;		// 四舍五入运算
	modf(dec, &dec);					// 取整
	dec /= factor;						// 小数点右移decimalnum位
	return num + dec;					// 合并返回
}

// 函数：整形数1-26转变成字符A-Z(大写英文字符)。
// 输入：INT i_ 整形数。
// 返回：A - Z 大写英文字符； 0x00 失败。
// 注释：整形数必须在1 - 26 之间,如果超过这个范围,返回错误值0x00。
CHAR knInt2Char(INT i_) {
	CHAR c;
	if(i_ <= 0 || i_ > 26) {
		knError("%s Line %d warning: Input i_ is invalid.\n", __FILE__, __LINE__);
		return 0x00;
	}
	c = (CHAR)i_ + 'A' - 1;
	return c;
}

// 函数：英文字符A-Z(大写英文字符)转变成整形数1-26。
// 输入：CHAR c_ 英文字符（大写）。
// 返回：序号1 - 26； -1 失败。
// 注释：只接收A - Z 大写英文字符。
INT knChar2Int(CHAR c_) {
	INT i;
	if((INT)c_ < 65 || (INT)c_ > 91) { // 英文字符A对应整形数是65,后续字符顺序增加1。
		knError("%s Line %d warning: Input c_ is invalid.\n", __FILE__, __LINE__);
		return -1;
	}
	i = (INT)c_ - 65 + 1;
	return i;
}
// ******************** End of 数字字符串转换 *********************************

// ***************** 浮点数网络字节序与主机字节序转换 *************************
// 函数：将浮点数float从主机字节序转变为网络字节序，并保存到knFParts中。
// 输入：float f_ 浮点数, knFParts &p_返回浮点数网络字节序数据结构(in-out)。
// 返回：无。
// 注释：测试通过。lxq20090731 20090813
void htonf(FLOAT f_, knFParts *p_) {
	int size = 1000; // 扩大1千倍。
	p_->integ = (LONG)f_;
 	p_->decim = (LONG)(f_*size - p_->integ*size)+1;
	p_->integ = htonl(p_->integ);
	p_->decim = htonl(p_->decim);
}

// 函数：将浮点数float从网络字节序转变为主机字节序。
// 输入：knFParts p_ 浮点数网络字节序数据结构, float &f_ 返回主机字节序浮点数(in-out)。
// 返回：0 成功；-1 失败。
// 注释：测试通过。lxq20090731 20090813
int ntohf(knFParts p_, FLOAT *f_) {
	LONG linteg, ldecim;
	FLOAT fdecim;
	int size = 1000; // 缩小1千倍。
	linteg = ntohl(p_.integ);
	ldecim = ntohl(p_.decim);
	fdecim = (FLOAT)ldecim / size;	// 返回原值。
	*f_ = (FLOAT)linteg + fdecim;
	return 0;
}

// 函数：将浮点数double从主机字节序转变为网络字节序，并保存到knFParts中。
// 输入：double f_ 浮点数, knFParts &p_返回浮点数网络字节序数据结构(in-out)。
// 返回：无。
// 注释：测试通过。lxq20090731 20090813
void htond(DOUBLE d_, knFParts *p_) {
	int size = 1000; // 扩大1千倍。
	p_->integ = (LONG)d_;
 	p_->decim = (LONG)(d_*size - p_->integ*size)+1;
	p_->integ = htonl(p_->integ);
	p_->decim = htonl(p_->decim);
}

// 函数：将浮点数double从网络字节序转变为主机字节序。
// 输入：knFParts p_ 浮点数网络字节序数据结构, float &f_ 返回主机字节序浮点数(in-out)。
// 返回：0 成功；-1 失败。
// 注释：测试通过。lxq20090731 20090813
int ntohd(knFParts p_, DOUBLE *d_) {
	LONG linteg, ldecim;
	DOUBLE ddecim;
	int size = 1000; // 缩小1千倍。
	linteg = ntohl(p_.integ);
	ldecim = ntohl(p_.decim);
	ddecim = (DOUBLE)ldecim / size;	// 返回原值。
	*d_ = (DOUBLE)linteg + ddecim;
	return 0;
}
// ****************** End of 浮点数网络字节序与主机字节序转换 ******************

// ****************** 网络字节序与主机字节序转换处理 ***************************
// 函数：测试当前操作系统数字型变量存储字节序。
// 输入：无。
// 返回：0 英特主机字节序 LittleEndian; 1 网络字节序 BigEndian。
// 注释：如果判断当前系统是主机字节序，则需要对网络传输数字数值进行字节序转换。
//           测试通过。lxq20070729
int knCheckEndian(void) {
	SHORT s = 0x1122;
	CHAR *p = (CHAR*)&s;
	if(p[0] == 0x11 && p[1] == 0x22) {			// 网络字节序 BigEndian
		p_knEndianStatus = KN_ENDIAN_NET;
		return KN_ENDIAN_NET;
	}
	else if(p[0] == 0x22 && p[1] == 0x11) {		// 主机字节序 LittleEndian
		p_knEndianStatus = KN_ENDIAN_HOST;
		return KN_ENDIAN_NET;
	}
	return -1;
}

// 函数：主机字节序数据到网络字节序数据的智能转换处理。
// 输入：void *inData_, 输入数据, CHAR type_ 输入数据类型, void *outData_ 返回数据(out)。
// 返回：无。
// 注释：本函数用于进行各种类型的数值数据主机字节序与网络字节序转换。
// 测试：测试通过。lxq20090811 20090813
void knhtonData(void *inData_, UCHAR type_, void *outData_) {
	SHORT	*pshortOut;	// short型输出数据指针。
	SHORT	 vshort;	// short型数据值。
	INT		*pintOut;	// int型输出数据指针。
	INT		 vint;		// int型数据值。
	LONG	*plongOut;	// long型输出数据指针。
	LONG	 vlong;		// long型数据值。
	if(p_knEndianStatus == KN_ENDIAN_HOST) {
		if(type_ == KN_SHORT || type_ == KN_USHORT) {		// short类型。short, usignend short
			pshortOut	= (SHORT*)outData_;
			vshort		= htons(*(SHORT*)inData_);
			*pshortOut = vshort;
		}
		else if(type_ == KN_INT || type_ == KN_UINT) {		// int类型 int, unsigned int
			pintOut		= (INT*)outData_;
			vint		= htonl(*(INT*)inData_);
			*pintOut	= vint;
		}
		else if(type_ == KN_LONG || type_ == KN_ULONG) {	// long类型 long, unsigend long
			plongOut	= (LONG*)outData_;
			vlong		= htonl(*(LONG*)inData_);
			*plongOut	= vlong;
		}
		else if(type_ == KN_FLOAT) {						// float类型 float
			htonf(*(FLOAT*)inData_, (knFParts*)outData_);
		}
		else if(type_ == KN_DOUBLE) {						// double类型 double
			htond(*(DOUBLE*)inData_, (knFParts*)outData_);
		}
		else {
			knError("%s Line %d warning: type_ is fault.\n", __FILE__, __LINE__);
			outData_ = NULL;
		}
	}
	else {
		outData_ = inData_;
	}
}

// 函数：网络字节序数据到主机字节序数据的智能转换处理。
// 输入：void *inData_, 输入数据, CHAR type_ 数据类型, void *outData_ 返回数据(out)。
// 返回：无。
// 注释：本函数用于进行各种类型的数值数据网络字节序与主机字节序转换。
// 测试：测试通过。lxq20090813
void knntohData(void *inData_, UCHAR type_, void *outData_) {
	SHORT	*pshortOut;	// short型输出数据指针。
	SHORT	 vshort;	// short型数据值。
	INT		*pintOut;	// int型输出数据指针。
	INT		 vint;		// int型数据值。
	LONG	*plongOut;	// long型输出数据指针。
	LONG	 vlong;		// long型数据值。
	if(p_knEndianStatus == KN_ENDIAN_HOST) {
		if(type_ == KN_SHORT || type_ == KN_USHORT) {		// short类型。short, usignend short
			pshortOut = (SHORT*)outData_;
			vshort = ntohs(*(SHORT*)inData_);
			*pshortOut = vshort;
		}
		else if(type_ == KN_INT || type_ == KN_UINT) {		// int类型 int, unsigned int
			pintOut = (INT*)outData_;
			vint = ntohl(*(INT*)inData_);
			*pintOut = vint;
		}
		else if(type_ == KN_LONG || type_ == KN_ULONG) {	// long类型 long, unsigend long
			plongOut = (LONG*)outData_;
			vlong = ntohl(*(LONG*)inData_);
			*plongOut = vlong;
		}
		else if(type_ == KN_FLOAT) {						// float类型 float
			ntohf(*(knFParts*)inData_, (FLOAT*)outData_);
		}
		else if(type_ == KN_DOUBLE) {						// double类型 double
			ntohd(*(knFParts*)inData_, (DOUBLE*)outData_);
		}
		else {
			knError("%s Line %d warning: type_ is fault.\n", __FILE__, __LINE__);
			outData_ = NULL;
		}
	}
	else {
		outData_ = inData_;
	}
}
// ****************** End of 网络字节序与主机字节序转换处理 *******************

// ****************** 系统信号 ************************************************
// 函数：安装系统信号对象。
// 输入：knSigCb *cb_ 信号回调函数指针；void *vparam_ 变量指针。
// 返回：无。
void knInstallSignal(knSigCb *cb_) {	// 信号在signal.h文件中声明。
	int i;						// 计数器。
	int sigList[] = {			// 信号列表。
#ifdef _LINUX32
		SIGFPE,		// Erroneous arithmetic operation.
		SIGHUP,		// Hangup.
		SIGILL,		// Illegal instruction.
		SIGPIPE,	// Write on a socket with no one to read
		SIGQUIT,	// Terminal quit signal.
		SIGTERM,	// Termination signal.
		SIGBUS,		// Bus error.
		SIGSYS,		// Bad system call.
#endif
#ifdef _WIN32
		SIGILL,		// illegal instruction - invalid function image
		SIGFPE,		// floating point exception
		SIGSEGV,	// segment violation
		SIGTERM,	// Software termination signal from kill
		SIGBREAK,	// Ctrl-Break sequence
		SIGABRT,	// abnormal termination triggered by abort call
		SIGINT,		// interrupt
#endif
		0
	};
	for(i = 0; sigList[i] != 0; i++) {
		signal(sigList[i], cb_);
	}
}
// ******************** End of 系统信号 ***************************************

// ******************** 目录操作 **********************************************
// 函数：获取当前目录
// 输入：char *dir_ 目录(in-out)。
// 返回：当前目录，NULL 失败。
// 注释：
char *knGetDir(char *dir_) {
	char *dir = NULL;
#ifdef _WIN32
	if(NULL == (dir = _getcwd(dir_, KN_LINE_MAXBYTES))) {
		knError("%s Line %d warning: _getcwd() failed.\n", __FILE__, __LINE__);
	}
#endif
#ifdef _LINUX32
	if(NULL == (dir = getcwd(dir_, KN_LINE_MAXBYTES))) {
		knError("%s Line %d warning: _getcwd() failed.\n", __FILE__, __LINE__);
	}
#endif
	return dir;
}

// 函数：创建目录
// 输入：char *dir_ 目录。
// 返回：0 成功，-1 失败。
// 注释：
INT knMkDir(char *dir_) {
	int ret;
#ifdef _WIN32
	if(-1 == (ret = _mkdir((const char*)dir_))) {
		knError("%s Line %d warning: _mkdir() failed.\n", __FILE__, __LINE__);
	}
#endif
#ifdef _LINUX32
	if(-1 == (ret = mkdir((const char*)dir_, S_IWUSR))) {
		knError("%s Line %d warning: mkdir() failed.\n", __FILE__, __LINE__);
	}
#endif
	return ret;
}

// 函数：删除目录
// 输入：char *dir_ 目录。
// 返回：0 成功，-1 失败。
// 注释：
INT knRmDir(char *dir_) {
#ifdef _WIN32
	if(-1 == (_rmdir((const char*)dir_))) {
		knError("%s Line %d warning: _rmdir() failed.\n", __FILE__, __LINE__);
	}
#endif
#ifdef _LINUX32
	if(-1 == remove(dir_)) {
		knError("%s Line %d warning: remove() failed.\n", __FILE__, __LINE__);
    }
#endif
	return 0;
}

// 函数：改变目录
// 输入：char *dir_ 目录。
// 返回：0 成功，-1 失败。
// 注释：
INT knChDir(char *dir_) {
	int ret;
#ifdef _WIN32
	if(-1 == (ret = _chdir((const char*)dir_))) {
		// 有时用于检测目录是否存在，不算错误，因此封闭本提示。
		//knError("%s Line %d warning: _chdir() failed for %s.\n", __FILE__, __LINE__, dir_);
	}
#endif
#ifdef _LINUX32
	if(-1 == (ret = chdir((const char*)dir_))) {
		// 有时用于检测目录是否存在，不算错误，因此封闭本提示。
		//knError("%s Line %d warning: chdir() failed for %s.\n", __FILE__, __LINE__, dir_);
	}
#endif
	return ret;
}
// ******************** End of 目录操作 ***************************************

// ******************** ping IP地址 *******************************************
// 函数：ping IP地址。
// 输入：CHAR *ip_ 目标IP地址。
// 返回：0 成功；-1失败。
// 注释：ping IP地址。
INT knPing(CHAR *ip_) {
	CHAR buf[128];			// 内存区。
	FILE *fp = NULL;		// 文件句柄。
	INT getFlag = 0;		// 没有获取IP地址标识：-1 没有Ping到IP地址；0 Ping到IP地址。
	INT i = 0;
	char *ip, *tmp;
	// ********** 检查输入的ip_是否合法 *********************
	ip = (char*)malloc(32);
	knBzero(ip, 32);
	memcpy(ip, ip_, 16);
	tmp = ip;
	for(;;) {
		tmp = knCutPrefix(tmp, '.');
		if(0 == strlen(tmp)) break;
		if(3 <= i++) break;
	}
	free(ip);
	if(3 != i) {
		knError("%s Line %d warning: Input ip_ is invalid.\n", __FILE__, __LINE__);
		return -1;
	}
	// ********** End of 检查输入的ip_是否合法 **************
	knBzero(buf, sizeof(buf));
#ifdef _WIN32
	knSprintf(buf, "ping %s -n 1 -l 1 -w 5 > iptemp", ip_);
#endif
#ifdef _LINUX32
    knSprintf(buf, "ping %s -c 1 -s 8 -w 5 > iptemp", ip_);
    return 0;
#endif
	system(buf);			// 执行ping。
	if(NULL == (fp = (FILE*)knFopen("iptemp", "r"))) {
		knError("%s Line %d warning: knFopen() failed.\n", __FILE__, __LINE__);
		return -1;
	}
	while(!feof(fp)) {		// 遍历文件内含行记录。
		knBzero(buf, sizeof(buf));
		if(NULL != fgets(buf, KN_LINE_MAXBYTES, fp)) {
			if(NULL != strstr(buf, "(100%")) {
				getFlag = -1;
				break;
			}
			else if(NULL != strstr(buf, "could not find host")) {
				getFlag = -1;
				break;
			}
			else if(NULL != strstr(buf, "Destination host unreachable")) {
				getFlag = -1;
				break;
			}
			else if(NULL != strstr(buf, "不到")) {
				getFlag = -1;
				break;
			}
		}
	}
	fclose(fp);				// 关闭配置文件。
	system("del iptemp");
	if(0 == getFlag) {
		return 0;
	}
	else {
		return -1;
	}
}
// ******************** End of ping IP地址 ************************************

// ******************** 杂项函数封装 ******************************************
// 函数：封装malloc() 。
// 输入：void INT size_ 申请内存字节数。
// 返回：内存指针；NULL 失败。
void *knMalloc(INT size_) {
	void *pt = NULL;
	INT size = 0;
	if(size_ <= 32) {
		size = 32;
	}
	else if(size_ > 32 && size_ <= 128) {
		size = 128;
	}
	else if(size_ > 128 && size_ <= 256) {
		size = 256;
	}
	else if(size_ > 256 && size_ <= 512) {
		size = 512;
	}
	else if(size_ > 512 && size_ <= 1024) {
		size = 1024;
	}
	else if(size_ > 1024 && size_ <= 2048) {
		size = 2048;
	}
	else if(size_ > 2048 && size_ <= 4096) {
		size = 4096;
	}
	else if(size_ > 4096 && size_ <= 8192) {
		size = 8192;
	}
	else if(size_ > 8192 && size_ <= 32768) {
		size = 32768;
	}
	else {
		size = size_;
	}
	if(NULL == (pt = malloc(size))) {				// 申请内存。
		knError("%s Line %d warnning: malloc() failed.\n", __FILE__, __LINE__);
		return NULL;
	}
	memset(pt, 0, size);							// 初始化。
	return pt;
}

// 函数：封装bezro().初始化内存。
// 输入：void *buf 内存地址, INT size_ 申请内存字节数.
// 返回：无.
// 注释：将申请字节数size_规整为8的倍数,并且进行初始化memset处理.
void knBzero(void *buf_, INT size_) {
#ifdef _WIN32
	memset(buf_, 0, size_);
#endif
#ifdef _LINUX32
	bzero(buf_, size_);
#endif
}
// ******************** End of 杂项函数封装 ***********************************

// ******************** 系统崩溃跟踪 ******************************************
#ifdef _WIN32
/**
 * 函数：程序崩溃处理函数(zhenghongxue 20150326)
 * 注释：
 */
long __stdcall knExceptionCb(struct _EXCEPTION_POINTERS* excp_) {
	//程序崩溃后会生成dmp文件，根据该文件可以定位问题区域。
	void* hFile = NULL;
	hFile = CreateFile(L".\\dumpfile.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if( hFile != INVALID_HANDLE_VALUE) {
		MINIDUMP_EXCEPTION_INFORMATION einfo;
		einfo.ThreadId = GetCurrentThreadId();
		einfo.ExceptionPointers = excp_;
		einfo.ClientPointers = FALSE;
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &einfo, NULL, NULL);
		CloseHandle(hFile);
	}
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

// 函数：系统崩溃跟踪。
// 输入：无。
// 返回：无。
// 注释：当程序由于某种原因崩溃后，能够定位跟踪出问题的程序位置。
void knCollapseTrace() {
#ifdef _WIN32
	SetUnhandledExceptionFilter(knExceptionCb);
#endif
#ifdef _LINUX32
	knPrintf("knBaseUtil::knCollapseTrace(line %d) is empty.\n", __LINE__);
#endif
}
// ******************** End of 系统崩溃跟踪 ***********************************
