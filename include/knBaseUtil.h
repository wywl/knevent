/************************ knBaseUtil.h *****************************************
** CopyRight ( C )  kernel2
** Classification:  knBaseUtil.h (原名knPrintf.h)
** Author&Date:     Xie Jianping  29/1/1999
** Reversion:       Xiaoqun Lee   30/5/2002 2008-08-28
** Description:     打印与错误信息处理块
**					2008-08-28 重写knPrintf文件.测试：内存泄露为0.
**					本程序非线程安全.
**					2009-10-26 结合windows 2005的安全要求,部分重写某些程序函数.
**					本函数是kernel2系统的最基础模块,因此,不但要提供Debug打印
**					输出功能,重要的数据类型宏定义,以及一些基础封装函数,都要包
**					含在这个文件中.
**					考虑到knPrintf模块承担的内容不仅限于Debug功能,还提供数据类
**					型,封装基础函数等功能,因此,将这个模块改名为knBaseUtil.
**					2010-05-12
**					校验函数封装由杨新峰提供。
**					本程序已经能够在Linux下编译执行。2011-04-21
*******************************************************************************/
#ifndef _KNBASEUTIL_H_INCLUDE_
#define _KNBASEUTIL_H_INCLUDE_

#ifdef __cplusplus
extern "C" {
#endif

// 编译条件宏定义。
#ifdef _WIN32
#ifdef _BASE_LIB_API	// 动态库接口宏定义。如果在预编译中定义编译宏_BASE_LIB_API_， 则凡是前缀KN_LIB_API都提供动态库接口。
#define KN_LIB_API	__declspec(dllexport)
#else
#define KN_LIB_API	//__declspec(dllimport)
#endif
#define	KN_SLASH	'\\'	// Windows斜杠
#endif
#ifdef linux
#define _LINUX32
#define _PTHREADS
#define KN_LIB_API
#define KN_SLASH	'/'		// Linux斜杠
#endif

// ************************ 数据类型宏定义 ************************************
// 备注：由于knPrintf是基础模块,因此,将一些基础数据类型宏定义在本文件中声明.

// kernel2数据类型宏定义.(注：CHAR类型)
#define	KN_BYTE			0x01	// 0  = byte				1个字节 (目前无效 lxq20111010)
#define	KN_SHORT		0x08	// 1  = short				2个字节
#define	KN_USHORT		0x09	// 2  = unsigned short		2个字节
#define	KN_INT			0x20	// 3  = int					4个字节
#define	KN_UINT			0x21	// 4  = unsigned int		4个字节
#define	KN_LONG			0x30	// 5  = long				4个字节
#define	KN_ULONG		0x31	// 6  = unsigned long		4个字节
#define	KN_FLOAT		0x40	// 7  = float				4个字节
#define	KN_DOUBLE		0x50	// 8  = double				8个字节
#define	KN_CHAR			0x60	// 9  = CHAR				1个字节
#define	KN_UCHAR		0x61	// 10 = usigned char		1个字节
#define	KN_STRING		0x70	// 11 = string				16个字节
#define	KN_CHARX		0x71	// 12 = char*				4个字节
#define	KN_UCHARX		0x72	// 13 = unsigned char*		4个字节
#define	KN_VOIDX		0x73	// 14 = void*				4个字节 (目前无效。lxq20111010)
#define KN_LLONG		0x74	// 15 = long long			8个字节
#define KN_ULLONG		0x75	// 16 = unsigned long long	8个字节
#define KN_DATE			0x76	// 17 = 格式：YYYY-MM-DD    12个字节
#define KN_TIME			0x77	// 18 = 格式：YYYY-MM-DD hh:mm 18个字节。
#define	KN_INVALID	0x7F	// 19

// kernel2数据类型宏定义. 32位系统中int与long字节数一样。
typedef          char   CHAR;		// 1 byte
typedef unsigned char   UCHAR;		// 1 byte
typedef          short  SHORT;		// 2 bytes	-32767 -- +32767
typedef unsigned short  USHORT;		// 2 bytes	0 -- +65535
typedef          int    INT;		// 4 bytes	21474836478 -- +21474836478
typedef unsigned int    UINT;		// 4 bytes  0 -- +21474836478
typedef          long   LONG;		// 4 bytes	-2147483648~2147483647
typedef unsigned long   ULONG;		// 4 bytes
typedef          float  FLOAT;		// 4 bytes
typedef          double DOUBLE;		// 8 bytes
typedef	long long		LLONG;		// 8 bytes
typedef unsigned long long ULLONG;	// 8 bytyes

// 过滤宏定义。
#define KN_STR_FILTER		1	// 过滤空格。
#define KN_STR_NO_FILTER	0	// 不过滤空格。

// 解析子串数据结构。
typedef KN_LIB_API struct knSubString {
	CHAR sPrevStr[32];		// 解析前一字符串；
	CHAR sAfterStr[256];	// 解析后余下的字符串。
} knSubStr;
// ************************ End of 数据类型宏定义 *****************************

// ************************ knPrintf ******************************************
// 打印设置宏定义.
#define KN_COMPLIE				  0		// 编译选项：0 Windows 2005；1 其他.
#define PRINT_MAXBYTESINLOG 1024000		// 日志文件的最大字节数.
#define PRINT_INPUTMAXBYTES    2048		// knPrintf与knError输入参数最大字节数.
										// 如果输入字符串大于本值,则出现严重Bug.
#define PRINT_OFF                 0		// 停止打印输出.
#define PRINT_ON                  1		// 打印输出到屏幕（不打印输出到文件）.
#define PRINT_TO_FILE             2		// 打印输出到文件与屏幕.
#define PRINT_TO_FILE_ONLY        3		// 只打印输出到文件.
#define PRINT_KNPRINTF            0		// knPrintf
#define PRINT_KNERROR             1		// knError
#define KN_WAIT_FOREVER	 0xFFFFFFFF		// 无限制等待.

// 文件行长度字节数.
// 备注：用于knConfFile.h文件读取文件行内容的最大字节数.
#define KN_LINE_MAXBYTES		256		// 配置文件文件中每行最大字节数.

// 影响knOperation程序。
#define KN_NOTDEFINED			  -1		// 没有定义 Operation。

// 数字型变量字节序宏定义。
#define KN_ENDIAN_NET		    0		// 网络字节序。
#define KN_ENDIAN_HOST		  1		// 主机字节序。
// ******************** 打印接口API *******************************************
/** 函数：打印信息输出.
输入：const CHAR *fmt_打印输入参数.
返回：无.
注释：受打印状态设置的影响.
类型：高层函数。
*/
extern KN_LIB_API void knPrintf(const CHAR *fmt_, ...);

/** 函数：错误信息输出.
输入：const CHAR *fmt_ 打印输入参数.
返回：无.
注释：不受打印状态设置的影响.
类型：高层函数。
*/
extern KN_LIB_API void knError(const CHAR *fmt_, ...);

/** 函数：日志信息输出。尚未实现。
输入：const CHAR *fmt_ 日志输入参数.
返回：无.
注释：受日志设置状态设置的影响.待开发.
类型：高层函数。
*/
extern KN_LIB_API void knLog(const CHAR *fmt_, ...);

/*  *********** knPrintf()与knError()括号内的的 % 变量符号说明。
	%c 字符
	%d 带符号整数
	%i 带符号整数
	%e 科学计数法, 使用小写"e"
	%E 科学计数法, 使用大写"E"
	%f 浮点数  在浮点数前可以定位整形数与小数点数的长度，例如：%4.2f
	%lf双浮点数
	%g 使用%e或%f中较短的一个
	%G 使用%E或%f中较短的一个
	%o 八进制
	%s 一串字符
	%u 无符号整数
	%x 无符号十六进制数, 用小写字母
	%X 无符号十六进制数, 用大写字母
	%p 一个指针
	%n 参数应该是一个指向一个整数的指针,指向的是字符数放置的位置
	%% 一个'%'符号
	一个位于一个%和格式化命令间的整数担当着一个最小字段宽度说明符,并且加上足够
	多的空格或0使输出足够长. 如果想填充0,在最小字段宽度说明符前放置0. 可以使用
	一个精度修饰符,它可以根据使用的格式代码而有不同的含义用%e, %E和 %f,精度修饰
	符指定想要的小数位数. 例如,
    %12.6f
	将会至少显示12位数字,并带有6位小数的浮点数.
	用%g和 %G, 精度修饰符决定显示的有效数的位数最大值.
	用%s,精度修饰符简单的表示一个最大的最大长度, 以补充句点前的最小字段长度.
	所有的printf()的输出都是右对齐的,除非你在%符号后放置了负号. 例如,
    %-12.4f
	将会显示12位字符,4位小数位的浮点数并且左对齐. 可以修改带字母l和h%d, %i,
	%o,%u和 %x 等类型说明符指定长型和短型数据类型 (例如 %hd 表示一个短整数). %e,
	%f和%g 类型说明符,可以在它们前面放置l指出跟随的是一个double. %g, %f和 %e 类
	型说明符可以置于字符'#'前保证出现小数点, 即使没有小数位. 带%x类型说明符的'#'
	字符的使用,表示显示十六进制数时应该带'0x'前缀. 带%o类型说明符的'#'字符的使用,
	表示显示八进制数时应该带一个'0'前缀.
*/
// ******************** End of 打印接口 ***************************************

// ******************** 打印设置 **********************************************
/** 函数：关闭打印屏幕与操作日志文件日志输出.
输入：无.
返回：本函数关闭knPrintf()功能，但是不能关闭knError()打印输出的错误日志文件功能。
类型：高层函数。
*/
extern KN_LIB_API void knPrintfOff(void);

/** 函数：打开打印屏幕输出(只输出到屏幕).
输入：无.
返回：无.
类型：高层函数。
*/
extern KN_LIB_API void knPrintfOn(void);

/** 函数：打开打印屏幕与操作日志文件日志输出.
输入：无.
返回：无.
类型：高层函数。
*/
extern KN_LIB_API void knPrintfToFile(void);

/** 函数：只打开打印操作文件日志输出.
输入：无.
返回：无.
类型：高层函数。
*/
extern KN_LIB_API void knPrintfToFileOnly(void);

/** 函数：变更操作日志名称与大小.
输入：CHAR *fname_: 文件名；UINT bytes_: 文件长度.
返回：0 成功；-1 失败.
类型：高层函数。
*/
extern KN_LIB_API INT knPrintfProcLogConf(const CHAR *fname_, const UINT bytes_);

/** 函数：变更错误日志名称与大小.
输入：const CHAR *fname_: 文件名；UINT bytes_: 文件长度.
返回：0 成功；-1 失败.
类型：高层函数。
*/
extern KN_LIB_API INT knPrintfErrorLogConf(const CHAR *fname_, const UINT bytes_);

/** 函数：变更打印输出设置.
输入：const INT printfStatus_ 打印输出模式：0 停止打印输出；
//		 1 打印输出到屏幕；2 打印输出到文件与屏幕；3 只打印输出到文件.
返回：赋值范围：0 - 3.
类型：高层函数。
*/
extern KN_LIB_API void knSetPrintfStatus(const INT status_);

/** 函数：获取操作日志名.
输入：CHAR *name_ 返回文件名(out).
返回：文件名.
注释：Java或C#调用时，形参可为NULL, 使用返回值。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knGetProcLogName(CHAR *name_);

/** 函数：获取错误日志名.
输入：CHAR *name_ 返回文件名(out).
返回：文件名。
注释：Java或C#调用时，形参可为NULL, 使用返回值。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knGetErrorLogName(CHAR *name_);

/** 函数：获取操作日志文件最大长度.
输入：无.
返回：文件长度.
类型：高层函数。
*/
extern KN_LIB_API INT knGetProcLogSize(void);

/** 函数：获取错误日志文件最大长度.
输入：无.
返回：文件长度.
类型：高层函数。
*/
extern KN_LIB_API INT knGetErrorLogSize(void);

/** 函数：设置操作日志名.
输入：CHAR *name_ 文件名.
返回：无.
类型：高层函数。
*/
extern KN_LIB_API void knSetProcLogName(CHAR *name_);

/** 函数：设置错误日志名.
输入：CHAR *name_ 文件名.
返回：无.
类型：高层函数。
*/
extern KN_LIB_API void knSetErrorLogName(CHAR *name_);

/** 函数：设置操作日志文件最大长度.
输入：UINT size_ 文件长度.
返回：无.
类型：高层函数。
*/
extern KN_LIB_API void knSetProcLogSize(UINT size_);

/** 函数：设置错误日志文件最大长度.
输入：UINT size_ 文件长度.
返回：无.
类型：高层函数。
*/
extern KN_LIB_API void knSetErrorLogSize(UINT size_);
// ******************** End of 打印设置 ***************************************

// ******************** 系统时间与休眠 ****************************************
/** 函数：获取系统时间.
输入：INT format_： 模式  0 全时格式(yyyy-mm-dd hh:mm:ss)；
		                      1 日期格式(yyyy-mm-dd)；
		                      2 时分格式(yyyy-mm-dd hh:mm).
	  CHAR *myTime_: 系统时间(in_out).
返回：时间字符串.
注释：Java与C#调用时，使用返回值。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API char *knSysDateTime(INT format_, CHAR *myTime_);

/** 函数：秒睡眠.
输入：INT seconds_秒,如果超时定义KN_WAIT_FOREVER,则无限等待.
返回：无.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API void knSleep(INT seconds_);

/** 函数：毫秒睡眠.
输入：INT milliseconds_毫秒,如果超时定义KN_WAIT_FOREVER,则无限等待.
返回：无.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API void knSleepM(INT milliseconds_);

/** 函数：时刻。
输入：无。
返回：-1 失败。
注释：获取当前时刻(秒计时时刻)。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API DOUBLE knTime(void);

/** 函数：时间间隔（秒间隔）。
输入：DOUBLE end_ 结束时间点, DOUBLE begin_ 开始时间点.
返回：时间间隔；-1 失败。
注释：本函数与knTime()结合使用。精度：精度只有秒级。
类型：高层函数。
*/
extern KN_LIB_API DOUBLE knDiffTime(DOUBLE end_, DOUBLE begin_);

/** 函数：时钟计时时刻。
输入：无。
返回：-1 失败。
注释：获取当前时刻时钟计时时刻。
      clock tick：时钟计时单元（而不把它叫做时钟滴答次数），一个时钟计时单元
      的时间长短是由CPU控制的。一个clock tick不是CPU的一个时钟周期，而是C/C++
      的一个基本计时单位。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API LONG knClock(void);

/** 函数：时间间隔。
输入：LONG end_ 结束时间点, LONG begin_ 开始时间点.
返回：时间间隔；-1 失败。
注释：本函数与knClock()结合使用。精度：精度豪秒级。
类型：高层函数。
*/
extern KN_LIB_API DOUBLE knDiffColck(LONG end_, LONG begin_);
// ******************** End of 系统时间与休眠 *********************************

// ******************** 磁盘文件操作 ******************************************
/** 函数：封装fopen() .打开一个磁盘文件.
输入：const CHAR *filename_ 文件, const CHAR *mode_ 文件打开模式.
      mode_ 赋值如下：
      "r" Opens for reading. If the file does not exist or cannot be found,
	    the fopen call fails.
      "w" Opens an empty file for writing. If the given file exists, its
	    contents are destroyed.
      "a" Opens for writing at the end of the file (appending) without removing
	    the EOF markerbefore writing new data to the file; creates the file first
	    if it doesn't exist.
      "r+" Opens for both reading and writing. (The file must exist.)
      "w+" Opens an empty file for both reading and writing. If the given file
	    exists, its contents are destroyed.
      "a+" Opens for reading and appending; the appending operation includes the
	    removal of the EOF marker before new data is written to the file and the
	    EOF marker is restored after writing is complete; creates the file first
	    if it doesn't exist.
返回：文件句柄,NULL 失败.
注释：打开的文件必须关闭，否则文件将被锁住，不允许再次打开。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API void *knFopen(const CHAR *filename_, const CHAR *mode_);

/** 函数：封装fread().读取文件.
输入：void *buf_ 读写到内存的内存文件指针, INT size_ 文件内存结构字节数,
//		 INT itemCount_ 文件行数(或记录数), void *file_ 磁盘文件句柄.
返回：文件item个数(例如,记录数)；-1失败.
注释：本函数对fread()做了改进,可以从返回的buf_中直接读取从文件句柄中获取的数据.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knFread(void *buf_, INT size_, INT itemCount_, void *file_);

/** 函数：封装fwrite().将内存文件写入磁盘文件.
输入：const void *buf_ 准备存盘的内存文件指针, INT size_ 文件内存结构字节数,
//		 INT count_ 文件行数(或记录数), void *file_ 磁盘文件句柄.
返回：文件item个数(例如,记录数)；-1 失败.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knFwrite(const void *buf_, INT size_, INT count_, void *file_);

/** 函数：封装fclose().关闭磁盘文件.
输入：void *file_ 文件句柄.
返回：0 成功；-1失败.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knFclose(void *file_);

/** 函数原型：读磁盘文件回调函数原型。
输入：char* 每行文件字符串内容, void * 输出结果。
*/
typedef KN_LIB_API int (knFileCB)(char*, void*);

/** 函数：读磁盘文件。
输入：char *file_ 文件名, knFileCB fcb_ 行内容处理回调函数, void *result_ 输出结果。
返回：0 成功；-1失败。
注释：读磁盘文件的内容，每行内容由fcb_处理。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API int knReadFile(const char *file_, knFileCB fcb_, void *result_);
// ******************** End of 磁盘文件操作 ***********************************

 // ******************** 获取目录下的文件 *************************************
// 本函数在knDataUtil中实现，函数名是knGetCurrFiles()。
// ******************** End of 获取目录下的文件 *******************************

// ******************** 单双字节转换 ******************************************
/** 函数：将单字符字符串转换为双字节字符串.
输入：CHAR *sbcs_ 单字节字符串(in), INT sbcsSize_ 单字节字符串字节数(in),
      CHAR *dbcs_ 双字节字符串(out), INT dbcsSize_ 双字节字符串字节数(in).
返回：双字节字符串；NULL失败.
注释：single-byte character set (SBCS) pages or double-byte character set (DBCS)
      Java或C#接口时，使用返回值。
      Linux函数有内存泄露嫌疑，待修改????
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API CHAR *knSBCS2DBCS(CHAR *sbcs_, INT sbcsSize, CHAR *dbcs_, INT dbcsSize_);

/** 函数：将双字符字符串转换为单字节字符串.
输入：CHAR *dbcs_ 双字节字符串(in), INT dbcsSize_ 双字节字符串字节数(in),
      CHAR *sbcs_ 单字节字符串(out), INT sbcsSize_ 单字节字符串字节数(in).
返回：单字节字符串；NULL失败.
注释：single-byte character set (SBCS) pages or double-byte character set (DBCS)
      Java或C#接口时，使用返回值。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API CHAR *knDBCS2SBCS(CHAR *dbcs_, INT dbcsSize_, CHAR *sbcs_, INT sbcsSize_);
// ******************** End of 单双字节转换 ***********************************

// ******************** 字符串操作函数封装 ************************************
/** 说明：Windows 2005声明：ANSI函数 strcpy()；strtok()；sprintf();fopen()；
		  vsprintf()
		  有安全隐患,并做了安全升级,针对Windows 2005的升级,特对上述函数封装,保
		  证 Windows 2005以上环境下编译与标准ANSI环境编译兼容.
*/

/** 函数：封装strcpy() .
输入：CHAR *dest_ 目标字符串, const CHAR *source_ 源字符串.
返回：复制字符串；NULL 失败.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API CHAR *knStrcpy(CHAR *dest_, const CHAR *source_);

/** 函数：封装strcat() ，将源字符串复制到目标字符串中。
输入：CHAR *dest_ 目标字符串, INT len_ 源字符总长度, const CHAR *source_ 源字符串.
返回：目标字符串.
注释：将源字符串复制到目标字符串中。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API CHAR *knStrcat(CHAR *dest_, INT len_, const CHAR *source_);

/** 函数：封装strcmp() .
输入：CHAR *str1_ 第一个比较字符串, onst CHAR *str2_ 第二个比较字符串.
返回：0 字符串相同，-1 字符串不同。
注释：
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knStrcmp(CHAR *str1_, const CHAR *str2_);

/** 函数：封装strlen() .
输入：CHAR *str_ 符串.
返回：字符串长度；-1 失败。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knStrlen(CHAR *str_);

/** 函数：封装strtok() .
输入：CHAR *token_ 字符串(in_out), const CHAR *delimit_ 分隔符,
	  CHAR **context_ 两次回调之间存储信息(只用于Windows 2005编译环境).
返回：指向下一子串, NULL 无子串了.
注释：如果截取到子串,并且分隔符在原字符串首位,则token_返回原先的字符串,返回值
      是原字符串被截取后的剩余子串；如果分隔符不在原字符串首位,则token_与返回
	  值均返回截取的分隔符前面子串(token_与返回值相同)；如果没有截取到子串,则
	  token_返回原字符串,返回值为NULL.使用本函数时请一定注意上述情况.
	  形参context_在调用本函数前定义变量是CHAR *context,实参格式&context.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API CHAR *knStrtok(CHAR *token_, const CHAR *delimit_, CHAR **context_);

/** 函数：封装sprintf()，将变参复制到buf_中。
输入：const CHAR *buf_ 输入字符串；......
返回：复制字符串字符数；-1 失败.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knSprintf(CHAR *buf_, const CHAR *fmt_, ...);

/** 函数：从一个字符串中查找是否有子串. 本函数对应的标准函数是strstr()。
输入：CHAR *str_ 目标字符串, CHAR *subStr_ 子字符串。
返回：子字符串在目标字符串中的起始位置；-1 没有找到。
类型：高层函数。
*/
extern KN_LIB_API INT knCheckStringFromString(CHAR *str_, CHAR *subStr_);

/** 函数：删除输入字符串的符号前的字串前缀.
输入：CHAR *buffer_ 输入字符串(in_out), CHAR symbol_ 前缀符号.
返回：处理后的字符串。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knCutPrefix(CHAR *buffer_, CHAR symbol_);

/** 函数：删除输入字符串的符号后的字串后缀.
输入：CHAR *buffer_ 输入字符串(in_out), CHAR symbol_ 后缀符号.
返回：处理后的字符串。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knCutPostfix(CHAR *buffer_, CHAR symbol_);

/** 函数：过滤字符处理.内含字符串去空格,Tab空格等优化处理.
输入：CHAR *str_: 输入字符串指针(in_out)；CHAR filter_: 过滤字符.
返回：处理后的字符串。
注释：默认过滤去除空格、Tab空格, \t,\n,以及“//”后面的注释信息.
类型：高层函数。
*/
extern KN_LIB_API CHAR *knFilterChar(CHAR *str_, CHAR filter_);

/** 函数：过滤字符处理.内含字符串去空格,Tab空格等优化处理.
输入：CHAR *str_: 输入字符串指针(in_out)；CHAR filter_: 过滤字符.
返回：处理后的字符串。
注释：默认过滤去除空格、Tab空格, \t,\n,以及定义的过滤字符filter_，
      不过滤“//”后的注释信息。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knFilterChar2(CHAR *str_, CHAR filter_);

/** 函数：删除输入字符串的前后或之间的空格,或Tab空格.
输入：CHAR *str_ 字符串指针(in_out).
返回: 处理后的字符串.
注释：由于本函数调用knFilterChar()实现本功能，因此，能够将字符串之间的空格全部删除。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knTrim(CHAR *str_);

/** 函数：只删除输入字符串的前后空格,或Tab空格，不删除字符串内部之间空格.
输入：CHAR *str_ 字符串指针(in_out).
返回:	处理后的字符串。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knCutStrSpace(CHAR *str_);

/** 函数：处理一个不规范的字符串,使每个词之间有一个空格.
输入：CHAR *str_ 输入字符串指针(in_out)。
返回：处理后的字符串。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knOptimizeString(CHAR *str_);

/** 函数：从字符串中获取Key Value。.
输入：CHAR *key_ 获取Key(in_out), CHAR *value_ 获取Value(in_out),
	  CHAR symbol_ Key Value分隔符(in), const CHAR *str_ 源字符串(in),
      INT mode_ 处理字符串模式：KN_STR_NO_FILTER 不过滤空格等；KN_STR_FILTER 过滤空格。
返回：0 成功；-1 失败
注释：本函数功能等同于knGetSubstringFromStr()。
类型：高层函数。
*/
extern KN_LIB_API INT knGetKeyValueFromStr(CHAR *key_, CHAR *value_, CHAR symbol_, const CHAR *str_, INT mode_);

/** 函数：将字符串str_从第一个分割符处分割为两个字符串。
输入：CHAR *prevStr_ 获取分割符之前的内容
      CHAR *afterStr_ 获取分割符之后的内容,
      CHAR symbol_ 分隔符(in),
      const CHAR *str_ 源字符串(in),
      INT mode_ 处理字符串模式：KN_STR_FILTER 过滤空格等；KN_STR_NO_FILTER 不过滤。
返回：0 成功；-1 失败；1 结束处理(循环处理时使用)。
注释：本函数可循环使用，直至将源字符串解析完毕，每循环一次，需要将afterStr_清空，
      并用afterStr_置换str_。
	  本函数功能等同于knGetKeyValueFromStr()。
类型：高层函数。
*/
extern KN_LIB_API INT knGetSubstringFromStr(CHAR * prevStr_, CHAR* afterStr_, CHAR symbol_, const CHAR *str_, INT mode_);

/** 函数：保存截取位置前字符串。
输入：string &s 字符串(in_out), INT pos_子串截取位置。
返回：处理后的字符串。
注释：无。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knCutSubStr(CHAR *s_, INT pos_);

/** 函数：处理半个汉字问题。
输入：string &s 字符串
返回：处理后的字符串。
注释：如果输入字符串内有唯一一个半个汉字，则用本方法处理，副作用是将半个汉字后的字符串全部删除，本函数一般用于处理字符串末尾的半个汉字。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knDeleteLastChar(CHAR *s_);

/** 函数：从后往前截取字符串。
输入：char *bgStr_ 截取前段子串, char *edStr_ 截取后段子串,  
      char *str_ 原始字符串, int len_ 原始字符串长度, char flag_ 分隔字符。
返回：0 成功；-1 失败。
注释：原始字符串默认最大长度len_ < KN_EXEC_PATH；
      bgStr_ 定义长度为KN_EXEC_PATH，edStr_ 长度为64字节。      
*/
extern KN_LIB_API int knCutStrFromEndToBegin(char *bgStr_, char *edStr_, char *str_, int len_, char flag_);

/** 函数：小写字符串转大写字符串。
输入：CHAR *str_ 小写字符串.
返回：大写字符串；NULL 失败。
注释：形参str_必须用变量引入，否则出错。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API CHAR *knStr2Upper(CHAR *str_);

/** 函数：大写字符串转小写字符串。
输入：CHAR *str_ 大写字符串.
返回：小写字符串；NULL 失败。
注释：形参str_必须用变量引入，否则出错。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API CHAR *knStr2Lower(CHAR *str_);

/** 函数：用新符号替换字符串中的旧符号。
输入：CHAR *str_ 字符串(in_out), INT len_ 字符串长度, CHAR before_ 原有的符号,
      CHAR after_ 替换的符号。
返回：输出字符串；-1 失败。
注释：在输入字串串中，有特定的符号需要用新符号替换。
类型：高层函数。
*/
extern KN_LIB_API INT knReplaceSymbol(CHAR *str_, INT len_, CHAR before_, CHAR after_);
// ******************** End of 字符串操作函数封装 *****************************

// ******************** 数字字符串转换 ****************************************
/** 函数：将整形数转变为字符串函数.
输入：INT in_ 输入整形数(in), CHAR *out_ 输出字符串(in_out).
返回：字符串.
注释：默认输出out_字符为10进制数字字符串,CHAR *out_字节长度为16字节.
	  支持short，unsigned short，int，unsigned int。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knItoa(INT in_, CHAR *out_);

/** 函数：将长整形数转变为字符串函数.
输入：LONG in_ 输入长整形数(in), CHAR *out_ 输出字符串(in_out).
返回：字符串.
注释：默认输入in_字符为10进制数字字符串,CHAR *out_字节长度为16字节.
类型：高层函数。
*/
extern KN_LIB_API CHAR *knLtoa(LONG in_, CHAR *out_);

/** 函数：将浮点数转变为字符串函数.
输入：FLOAT in_ 输入浮点数(in), INT p_ 保留小数点，CHAR *out_ 输出字符串(in_out).
返回：字符串.
注释：默认输入in_字符为10进制数字字符串,CHAR *out_字节长度为32字节.
注释：Linux下小数点2位后不准确。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knFtoa(FLOAT in_, INT p_, CHAR *out_);

/** 函数：将双精浮点数转变为字符串函数.
输入：DOUBLE in_ 输入双精浮点数(in), INT p_ 保留小数点，CHAR *out_ 输出字符串(in_out).
返回：字符串.
注释：默认输入in_字符为10进制数字字符串,CHAR *out_字节长度为32字节.
	  out_ 不可为NULL。
类型：高层函数。
*/
extern KN_LIB_API CHAR *knDtoa(DOUBLE in_, INT p_, CHAR *out_);

/** 函数：将字符串整形数转变为INT型整形数函数.
输入：CHAR *in_ 输入字符串, INT *out_ 输出整形数指针（in-out）.
返回：整形数；-1 失败.
注释：默认输入in_字符为10进制数字字符串.
	  out_ 可为NULL。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knAtoi(CHAR *in_, INT *out_);

/** 函数：将字符串长整形数转变为LONG型整形数函数.
输入：CHAR *in_ 输入字符串, LONG *out_ 输出整形数指针.
返回：长整形数；-1 失败.
注释：默认输入in_字符为10进制数字字符串.
	  out_ 可为NULL。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API LONG knAtol(CHAR *in_, LONG *out_);

/** 函数：将字符串浮点数转变为FLOAT型数函数.
输入：CHAR *in_ 输入字符串, FLOAT *out_ 输出整形数指针.
返回：长整形数；-1 失败.
注释：默认输入in_字符为10进制数字字符串.
      Linux环境下，小数点2位数以后不准确，原因不清。
	  out_ 可为NULL。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API FLOAT knAtof(CHAR *in_, FLOAT *out_);

/** 函数：将字符串双精浮点数转变为DOUBLE型数函数.
输入：CHAR *in_ 输入字符串, DOUBLE *out_ 输出整形数指针.
返回：长整形数；-1 失败.
注释：默认输入in_字符为10进制数字字符串.
	  out_ 可为NULL。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API DOUBLE knAtod(CHAR *in_, DOUBLE *out_);

/** 函数：封装四舍五入函数.
输入：DOUBLE in_ 需要四舍五入的浮点数, INT decimal_ 保留有效小数点位数.
返回：四舍五入结果；-1 失败.
注释：返回四舍五入的结果浮点数.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API DOUBLE knRound(DOUBLE in_, INT decimal_);

/** 函数：整形数1-26转变成英文字符A-Z(大写英文字符).
输入：INT i_ 整形数.
返回：A - Z 大写英文字符； 0x00 失败.
注释：整形数必须在1 - 26 之间,如果超过这个范围,返回错误值0x00.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API CHAR knInt2Char(INT i_);

/** 函数：英文字符A-Z(大写英文字符)转变成整形数1-26.
输入：CHAR c_ 英文字符（大写）.
返回：序号1 - 26； -1 失败.
注释：只接收A - Z 大写英文字符.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knChar2Int(CHAR c_);
// ******************** End of 数字字符串转换 *********************************

// ***************** 网络字节序与主机字节序转换处理 ***************************
// 数据结构：浮点数(float/double)网络字节序保存结构。
// 注释：浮点数转换成整形部分与小数点部分似乎不是一个最好的方法，因为double的长度
//       是8,而这种做法，人为限制整形部分与小数点部分平均各4个字节长度，有可能对
//		 数据精度有影响。如果转成字符串似乎更可取。lxq20090828
typedef struct knFloatParts {
	LONG  integ;    // 浮点数的整数部分。
	LONG  decim;    // 浮点数的小数部分。小数点值乘1000保存在此。
} knFParts;

/** 函数：测试当前操作系统数字型变量存储字节序。
输入：无。
返回：0 英特主机字节序 LittleEndian; 1 网络字节序 BigEndian。
注释：如果判断当前系统是主机字节序，则需要对网络传输数字数值进行字节序转换。
类型：高层函数。
*/
extern KN_LIB_API INT knCheckEndian(void);

/** 函数：主机字节序数据到网络字节序数据的智能转换处理。
输入：void *inData_, 输入数据, CHAR type_ 输入数据类型, void *outData_
	  返回数据(out)。
返回：无。
注释：本函数用于进行各种类型的数值数据主机字节序与网络字节序转换。
	  type_: KN_SHORT，KN_USHORT，KN_INT，KN_UINT，KN_LONG，KN_ULONG，
	  KN_FLOAT，KN_DOUBLE
	  如果数据类型是浮点数KN_FLOAT与KNDOUBLE，则输出数据为knFParts实例,
	  且精度只能保存小数点后3位。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API void knhtonData(void *inData_, UCHAR type_, void *outData_);

/** 函数：网络字节序数据到主机字节序数据的智能转换处理。
输入：void *inData_, 输入数据, CHAR type_ 数据类型, void *outData_
	  返回数据(out)。
返回：无。
注释：本函数用于进行各种类型的数值数据网络字节序与主机字节序转换。
	  type_: KN_SHORT，KN_USHORT，KN_INT，KN_UINT，KN_LONG，KN_ULONG，
	  KN_FLOAT，KN_DOUBLE。
	  如果数据类型是浮点数KN_FLOAT与KNDOUBLE，则输入数据为knFParts实例,
	  且精度只能保存小数点后3位。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API void knntohData(void *inData_, UCHAR type_, void *outData_);
// ***************** End of 网络字节序与主机字节序转换处理 ********************

// ******************** 系统信号 **********************************************
// 信号回调函数
typedef KN_LIB_API void (knSigCb)(int);

/** 函数：安装消息信号对象。
输入：无。
返回：无。
注释：本函数用于knESB与knXXX进程模块中。
类型：底层封装函数。暂不支持Linux。李小群2015-05-12
*/
extern KN_LIB_API void knInstallSignal(knSigCb *cb_);
// ******************** End of 系统信号 ***************************************

// ******************** 目录操作 **********************************************
/** 函数：获取当前目录
输入：char *dir_ 目录(in-out)。
返回：当前目录，NULL 失败。
注释：dir_默认字节数为KN_LINE_MAXBYTES。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API CHAR *knGetDir(char *dir_);

/** 函数：创建目录
输入：char *dir_ 目录。
返回：0 成功，-1 失败。
注释：Linux没有实现，李小群2015-05-09检查。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API int knMkDir(char *dir_);

/** 函数：删除目录
输入：char *dir_ 目录。
返回：0 成功，-1 失败。
注释：Linux没有实现，李小群2015-05-09检查。
类型：底层封装函数。支持Windows与Linux。
*/
 extern KN_LIB_API INT knRmDir(char *dir_);

/** 函数：改变目录
输入：char *dir_ 目录。
返回：0 成功，-1 失败。
注释：Linux没有实现，李小群2015-05-09检查。
类型：底层封装函数。支持Windows与Linux。
*/
 extern KN_LIB_API int knChDir(char *dir_);
// ******************** End of 目录操作 ***************************************

// ******************** ping IP地址 *******************************************
/** 函数：ping IP地址。  还未实现Linux下的Ping操作，待完成。
输入：CHAR *ip_ 目标IP地址。
返回：0 成功；-1失败。
注释：ping IP地址，必须具备在的确目录下读写文件的权限。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API INT knPing(CHAR *ip_);
// ******************** End of ping IP地址 ************************************

// ******************** 杂项函数封装 ******************************************
/** 函数：封装malloc().申请内存。
输入：INT size_ 申请内存字节数.
返回：内存指针；NULL 失败.
注释：将申请字节数size_规整为8的倍数,并且进行初始化memset处理。
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API void *knMalloc(INT size_);
/** 函数：封装bezro().初始化内存。
输入：void *buf 内存地址, INT size_ 申请内存字节数.
返回：无.
注释：将申请字节数size_规整为8的倍数,并且进行初始化memset处理.
类型：底层封装函数。支持Windows与Linux。
*/
extern KN_LIB_API void knBzero(void *buf_, INT size_);
// ******************** End of 杂项函数封装 ***********************************

// ******************** 系统崩溃跟踪 ******************************************
/** 函数：系统崩溃跟踪。
输入：无。
返回：无。
注释：当程序由于某种原因崩溃后，能够定位跟踪出问题的程序位置。郑红学开发。
      本函数目前只用于Windows程序崩溃跟踪，要求调试信息格式设为:程序数据库 (/Zi)
类型：底层封装函数。暂不支持Linux。李小群2015-05-12
*/
extern KN_LIB_API void knCollapseTrace();
// ******************** End of 系统崩溃跟踪 ***********************************
// ******************** End of knBaseUtil *************************************
#ifdef __cplusplus
}
#endif
#endif	// _KNBASEUTIL_H_INCLUDE_
