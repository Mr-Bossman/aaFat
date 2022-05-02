
#ifndef __AAFAT_CONF_H
#define __AAFAT_CONF_H
//#define EXAMPLE_

//#define BACKTRACE_ERR
//#define BT_SZ 100
#define BLOCK_SIZE 0x8000
#define TABLE_LEN (BLOCK_SIZE/4)
#else
#ifndef EXAMPLE_
#define EXAMPLE_
#endif
#endif /* __AAFAT_CONF_H */
