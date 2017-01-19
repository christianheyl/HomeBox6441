#ifndef __LOG_PUREDB_H__
#define __LOG_PUREDB_H__ 1

#ifdef 	ARC_PW_PRIVILEGE
void pw_puredb_check(AuthResult * const result,
                     const char *account, const char *password,
                     const struct sockaddr_storage * const sa,
                     const struct sockaddr_storage * const peer,
					 const int nTlsCnt);
#else
void pw_puredb_check(AuthResult * const result,
                     const char *account, const char *password,
                     const struct sockaddr_storage * const sa,
                     const struct sockaddr_storage * const peer);
#endif // ARC_PW_PRIVILEGE

void pw_puredb_parse(const char * const file);

void pw_puredb_exit(void);

#endif
