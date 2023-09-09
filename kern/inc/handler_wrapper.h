/**
 * @file handler_wrapper.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief assembly wrappers for interrupt handlers
 */

#ifndef HANDLER_WRAPPER_H_SEEN
#define HANDLER_WRAPPER_H_SEEN

#include <ureg.h> // ureg_t

/**
 * @brief Wrap the user provided handler, which is earlier registered
 *        via the swexn system call.
 * 
 * @param ureg_ptr pointer to the register values which are snapshotted
 *                 before the interrupt happens.
 * @param exception_stack the stack which the user provided handler
 *                        should be run on
 * @param handler the user provided handler
 * @param arg the opaque argument provided to swexn
 */
void wrap_user_provided_handler(
    ureg_t *ureg_ptr,
    void *exception_stack,
    void (*handler)(void *arg, ureg_t *ureg_ptr),
    void *arg
);

// handler wrappers prepared for every interrupt vector
void wrap_handler0(void);
void wrap_handler1(void);
void wrap_handler2(void);
void wrap_handler3(void);
void wrap_handler4(void);
void wrap_handler5(void);
void wrap_handler6(void);
void wrap_handler7(void);
void wrap_handler8(void);
void wrap_handler9(void);
void wrap_handler10(void);
void wrap_handler11(void);
void wrap_handler12(void);
void wrap_handler13(void);
void wrap_handler14(void);
void wrap_handler15(void);
void wrap_handler16(void);
void wrap_handler17(void);
void wrap_handler18(void);
void wrap_handler19(void);
void wrap_handler20(void);
void wrap_handler21(void);
void wrap_handler22(void);
void wrap_handler23(void);
void wrap_handler24(void);
void wrap_handler25(void);
void wrap_handler26(void);
void wrap_handler27(void);
void wrap_handler28(void);
void wrap_handler29(void);
void wrap_handler30(void);
void wrap_handler31(void);
void wrap_handler32(void);
void wrap_handler33(void);
void wrap_handler34(void);
void wrap_handler35(void);
void wrap_handler36(void);
void wrap_handler37(void);
void wrap_handler38(void);
void wrap_handler39(void);
void wrap_handler40(void);
void wrap_handler41(void);
void wrap_handler42(void);
void wrap_handler43(void);
void wrap_handler44(void);
void wrap_handler45(void);
void wrap_handler46(void);
void wrap_handler47(void);
void wrap_handler48(void);
void wrap_handler49(void);
void wrap_handler50(void);
void wrap_handler51(void);
void wrap_handler52(void);
void wrap_handler53(void);
void wrap_handler54(void);
void wrap_handler55(void);
void wrap_handler56(void);
void wrap_handler57(void);
void wrap_handler58(void);
void wrap_handler59(void);
void wrap_handler60(void);
void wrap_handler61(void);
void wrap_handler62(void);
void wrap_handler63(void);
void wrap_handler64(void);
void wrap_handler65(void);
void wrap_handler66(void);
void wrap_handler67(void);
void wrap_handler68(void);
void wrap_handler69(void);
void wrap_handler70(void);
void wrap_handler71(void);
void wrap_handler72(void);
void wrap_handler73(void);
void wrap_handler74(void);
void wrap_handler75(void);
void wrap_handler76(void);
void wrap_handler77(void);
void wrap_handler78(void);
void wrap_handler79(void);
void wrap_handler80(void);
void wrap_handler81(void);
void wrap_handler82(void);
void wrap_handler83(void);
void wrap_handler84(void);
void wrap_handler85(void);
void wrap_handler86(void);
void wrap_handler87(void);
void wrap_handler88(void);
void wrap_handler89(void);
void wrap_handler90(void);
void wrap_handler91(void);
void wrap_handler92(void);
void wrap_handler93(void);
void wrap_handler94(void);
void wrap_handler95(void);
void wrap_handler96(void);
void wrap_handler97(void);
void wrap_handler98(void);
void wrap_handler99(void);
void wrap_handler100(void);
void wrap_handler101(void);
void wrap_handler102(void);
void wrap_handler103(void);
void wrap_handler104(void);
void wrap_handler105(void);
void wrap_handler106(void);
void wrap_handler107(void);
void wrap_handler108(void);
void wrap_handler109(void);
void wrap_handler110(void);
void wrap_handler111(void);
void wrap_handler112(void);
void wrap_handler113(void);
void wrap_handler114(void);
void wrap_handler115(void);
void wrap_handler116(void);
void wrap_handler117(void);
void wrap_handler118(void);
void wrap_handler119(void);
void wrap_handler120(void);
void wrap_handler121(void);
void wrap_handler122(void);
void wrap_handler123(void);
void wrap_handler124(void);
void wrap_handler125(void);
void wrap_handler126(void);
void wrap_handler127(void);
void wrap_handler128(void);
void wrap_handler129(void);
void wrap_handler130(void);
void wrap_handler131(void);
void wrap_handler132(void);
void wrap_handler133(void);
void wrap_handler134(void);
void wrap_handler135(void);
void wrap_handler136(void);
void wrap_handler137(void);
void wrap_handler138(void);
void wrap_handler139(void);
void wrap_handler140(void);
void wrap_handler141(void);
void wrap_handler142(void);
void wrap_handler143(void);
void wrap_handler144(void);
void wrap_handler145(void);
void wrap_handler146(void);
void wrap_handler147(void);
void wrap_handler148(void);
void wrap_handler149(void);
void wrap_handler150(void);
void wrap_handler151(void);
void wrap_handler152(void);
void wrap_handler153(void);
void wrap_handler154(void);
void wrap_handler155(void);
void wrap_handler156(void);
void wrap_handler157(void);
void wrap_handler158(void);
void wrap_handler159(void);
void wrap_handler160(void);
void wrap_handler161(void);
void wrap_handler162(void);
void wrap_handler163(void);
void wrap_handler164(void);
void wrap_handler165(void);
void wrap_handler166(void);
void wrap_handler167(void);
void wrap_handler168(void);
void wrap_handler169(void);
void wrap_handler170(void);
void wrap_handler171(void);
void wrap_handler172(void);
void wrap_handler173(void);
void wrap_handler174(void);
void wrap_handler175(void);
void wrap_handler176(void);
void wrap_handler177(void);
void wrap_handler178(void);
void wrap_handler179(void);
void wrap_handler180(void);
void wrap_handler181(void);
void wrap_handler182(void);
void wrap_handler183(void);
void wrap_handler184(void);
void wrap_handler185(void);
void wrap_handler186(void);
void wrap_handler187(void);
void wrap_handler188(void);
void wrap_handler189(void);
void wrap_handler190(void);
void wrap_handler191(void);
void wrap_handler192(void);
void wrap_handler193(void);
void wrap_handler194(void);
void wrap_handler195(void);
void wrap_handler196(void);
void wrap_handler197(void);
void wrap_handler198(void);
void wrap_handler199(void);
void wrap_handler200(void);
void wrap_handler201(void);
void wrap_handler202(void);
void wrap_handler203(void);
void wrap_handler204(void);
void wrap_handler205(void);
void wrap_handler206(void);
void wrap_handler207(void);
void wrap_handler208(void);
void wrap_handler209(void);
void wrap_handler210(void);
void wrap_handler211(void);
void wrap_handler212(void);
void wrap_handler213(void);
void wrap_handler214(void);
void wrap_handler215(void);
void wrap_handler216(void);
void wrap_handler217(void);
void wrap_handler218(void);
void wrap_handler219(void);
void wrap_handler220(void);
void wrap_handler221(void);
void wrap_handler222(void);
void wrap_handler223(void);
void wrap_handler224(void);
void wrap_handler225(void);
void wrap_handler226(void);
void wrap_handler227(void);
void wrap_handler228(void);
void wrap_handler229(void);
void wrap_handler230(void);
void wrap_handler231(void);
void wrap_handler232(void);
void wrap_handler233(void);
void wrap_handler234(void);
void wrap_handler235(void);
void wrap_handler236(void);
void wrap_handler237(void);
void wrap_handler238(void);
void wrap_handler239(void);
void wrap_handler240(void);
void wrap_handler241(void);
void wrap_handler242(void);
void wrap_handler243(void);
void wrap_handler244(void);
void wrap_handler245(void);
void wrap_handler246(void);
void wrap_handler247(void);
void wrap_handler248(void);
void wrap_handler249(void);
void wrap_handler250(void);
void wrap_handler251(void);
void wrap_handler252(void);
void wrap_handler253(void);
void wrap_handler254(void);
void wrap_handler255(void);

#endif // HANDLER_WRAPPER_H_SEEN