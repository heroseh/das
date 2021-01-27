#ifdef __cplusplus
extern "C" {
#endif

#define test_iteration_count 2048


#define STRUCT_LIST \
	STRUCT(Test0) \
	STRUCT(Test1) \
	STRUCT(Test2) \
	STRUCT(Test3) \
	STRUCT(Test4) \
	STRUCT(Test5) \
	STRUCT(Test6) \
	STRUCT(Test7) \
	STRUCT(Test8) \
	STRUCT(Test9) \
	STRUCT(Test10) \
	STRUCT(Test11) \
	STRUCT(Test12) \
	STRUCT(Test13) \
	STRUCT(Test14) \
	STRUCT(Test15) \
	STRUCT(Test16) \
	STRUCT(Test17) \
	STRUCT(Test18) \
	STRUCT(Test19) \
	STRUCT(Test20) \
	STRUCT(Test21) \
	STRUCT(Test22) \
	STRUCT(Test23) \
	STRUCT(Test24) \
	STRUCT(Test25) \
	STRUCT(Test26) \
	STRUCT(Test27) \
	STRUCT(Test28) \
	STRUCT(Test29) \
	STRUCT(Test30) \
	STRUCT(Test31) \
	STRUCT(Test32) \
	STRUCT(Test33) \
	STRUCT(Test34) \
	STRUCT(Test35) \
	STRUCT(Test36) \
	STRUCT(Test37) \
	STRUCT(Test38) \
	STRUCT(Test39) \
	STRUCT(Test40) \
	STRUCT(Test41) \
	STRUCT(Test42) \
	STRUCT(Test43) \
	STRUCT(Test44) \
	STRUCT(Test45) \
	STRUCT(Test46) \
	STRUCT(Test47) \
	STRUCT(Test48) \
	STRUCT(Test49) \
	STRUCT(Test50) \
	STRUCT(Test51) \
	STRUCT(Test52) \
	STRUCT(Test53) \
	STRUCT(Test54) \
	STRUCT(Test55) \
	STRUCT(Test56) \
	STRUCT(Test57) \
	STRUCT(Test58) \
	STRUCT(Test59) \
	STRUCT(Test60) \
	STRUCT(Test61) \
	STRUCT(Test62) \
	STRUCT(Test63) \
	STRUCT(Test64) \
	STRUCT(Test65) \
	STRUCT(Test66) \
	STRUCT(Test67) \
	STRUCT(Test68) \
	STRUCT(Test69) \
	STRUCT(Test70) \
	STRUCT(Test71) \
	STRUCT(Test72) \
	STRUCT(Test73) \
	STRUCT(Test74) \
	STRUCT(Test75) \
	STRUCT(Test76) \
	STRUCT(Test77) \
	STRUCT(Test78) \
	STRUCT(Test79) \
	STRUCT(Test80) \
	STRUCT(Test81) \
	STRUCT(Test82) \
	STRUCT(Test83) \
	STRUCT(Test84) \
	STRUCT(Test85) \
	STRUCT(Test86) \
	STRUCT(Test87) \
	STRUCT(Test88) \
	STRUCT(Test89) \
	STRUCT(Test90) \
	STRUCT(Test91) \
	STRUCT(Test92) \
	STRUCT(Test93) \
	STRUCT(Test94) \
	STRUCT(Test95) \
	STRUCT(Test96) \
	STRUCT(Test97) \
	STRUCT(Test98) \
	STRUCT(Test99) \
	STRUCT(Test100) \
	STRUCT(Test101) \
	STRUCT(Test102) \
	STRUCT(Test103) \
	STRUCT(Test104) \
	STRUCT(Test105) \
	STRUCT(Test106) \
	STRUCT(Test107) \
	STRUCT(Test108) \
	STRUCT(Test109) \
	STRUCT(Test110) \
	STRUCT(Test111) \
	STRUCT(Test112) \
	STRUCT(Test113) \
	STRUCT(Test114) \
	STRUCT(Test115) \
	STRUCT(Test116) \
	STRUCT(Test117) \
	STRUCT(Test118) \
	STRUCT(Test119) \
	STRUCT(Test120) \
	STRUCT(Test121) \
	STRUCT(Test122) \
	STRUCT(Test123) \
	STRUCT(Test124) \
	STRUCT(Test125) \
	STRUCT(Test126) \
	STRUCT(Test127) \
	STRUCT(Test128) \
	STRUCT(Test129) \
	/* end */

#define STRUCT(name) typedef struct { int a, b, c, d; } name;
	STRUCT_LIST
#undef STRUCT

#ifdef __cplusplus
}
#endif

