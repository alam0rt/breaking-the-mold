typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;
typedef s32 bool;
typedef s16 fixed16;
typedef s32 fixed32;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
struct ToT {
 unsigned long *head;
 long size;
};
struct TCBH {
 struct TCB *entry;
 long flag;
};
struct TCB {
 long status;
 long mode;
 unsigned long reg[40];
 long system[6];
};
struct EvCB {
 unsigned long desc;
 long status;
 long spec;
 long mode;
 long (*FHandler)();
 long system[2];
};
struct EXEC {
        unsigned long pc0;
        unsigned long gp0;
        unsigned long t_addr;
        unsigned long t_size;
        unsigned long d_addr;
        unsigned long d_size;
        unsigned long b_addr;
        unsigned long b_size;
 unsigned long s_addr;
 unsigned long s_size;
 unsigned long sp,fp,gp,ret,base;
};
struct XF_HDR {
 char key[8];
 unsigned long text;
 unsigned long data;
 struct EXEC exec;
 char title[60];
};
struct DIRENTRY {
 char name[20];
 long attr;
 long size;
 struct DIRENTRY *next;
 long head;
 char system[4];
};
extern struct ToT SysToT[32];
extern long SysClearRCnt[];
extern long SetRCnt(unsigned long, unsigned short, long);
extern long GetRCnt(unsigned long);
extern long ResetRCnt(unsigned long);
extern long StartRCnt(unsigned long);
extern long StopRCnt(unsigned long);
extern long OpenEvent(unsigned long,long,long,long (*func)());
extern long CloseEvent(long);
extern long WaitEvent(long);
extern long TestEvent(long);
extern long EnableEvent(long);
extern long DisableEvent(long);
extern void DeliverEvent(unsigned long, unsigned long);
extern void UnDeliverEvent(unsigned long, unsigned long);
extern long OpenTh(long (*func)(), unsigned long , unsigned long);
extern int CloseTh(long);
extern int ChangeTh(long);
extern long open(char *, unsigned long);
extern long close(long);
extern long lseek(long, long, long);
extern long read(long, void *, long);
extern long write(long, void *, long);
extern long ioctl(long, long, long);
extern struct DIRENTRY * firstfile(char *, struct DIRENTRY *);
extern struct DIRENTRY * nextfile(struct DIRENTRY *);
extern long erase(char *);
extern long undelete(char *);
extern long format(char *);
extern long rename(char *, char *);
extern long cd(char *);
extern long LoadTest(char *, struct EXEC *);
extern long Load(char *, struct EXEC *);
extern long Exec(struct EXEC *, long, char **);
extern long LoadExec(char *, unsigned long, unsigned long);
extern long InitPAD(char *,long ,char *,long);
extern long StartPAD(void);
extern void StopPAD(void);
extern void EnablePAD(void);
extern void DisablePAD(void);
extern void FlushCache(void);
extern void ReturnFromException(void);
extern int EnterCriticalSection(void);
extern void ExitCriticalSection(void);
extern void Exception(void);
extern void SwEnterCriticalSection(void);
extern void SwExitCriticalSection(void);
extern unsigned long SetSp(unsigned long);
extern unsigned long GetSp( void );
extern unsigned long GetGp( void );
extern unsigned long GetCr( void );
extern unsigned long GetSr( void );
extern unsigned long GetSysSp(void);
extern long SetConf(unsigned long,unsigned long,unsigned long);
extern void GetConf(unsigned long *,unsigned long *,unsigned long *);
extern long _get_errno(void);
extern long _get_error(long);
extern void SystemError( char, long);
extern void SetMem(long);
extern long Krom2RawAdd( unsigned long );
extern long Krom2RawAdd2(unsigned short);
extern void _96_init(void);
extern void _96_remove(void);
extern void _boot(void);
extern void ChangeClearPAD( long );
extern void InitCARD(long val);
extern long StartCARD(void);
extern long StopCARD(void);
extern void _bu_init(void);
extern long _card_info(long chan);
extern long _card_clear(long chan);
extern long _card_load(long chan);
extern long _card_auto(long val);
extern void _new_card(void);
extern long _card_status(long drv);
extern long _card_wait(long drv);
extern unsigned long _card_chan(void);
extern long _card_write(long chan, long block, unsigned char *buf);
extern long _card_read(long chan, long block, unsigned char *buf);
extern long _card_format(long chan);
typedef struct {
 short m[3][3];
        long t[3];
} MATRIX;
typedef struct {
 long vx, vy;
 long vz, pad;
} VECTOR;
typedef struct {
 short vx, vy;
 short vz, pad;
} SVECTOR;
typedef struct {
 u_char r, g, b, cd;
} CVECTOR;
typedef struct {
 short vx, vy;
} DVECTOR;
typedef struct {
 SVECTOR v;
 VECTOR sxyz;
 DVECTOR sxy;
 CVECTOR rgb;
 short txuv,pad;
 long chx,chy;
} EVECTOR;
typedef struct {
 SVECTOR v;
 u_char uv[2]; u_short pad;
 CVECTOR c;
 DVECTOR sxy;
 u_long sz;
} RVECTOR;
typedef struct {
 RVECTOR r01,r12,r20;
 RVECTOR *r0,*r1,*r2;
 u_long *rtn;
} CRVECTOR3;
typedef struct {
 u_long ndiv;
 u_long pih,piv;
 u_short clut,tpage;
 CVECTOR rgbc;
 u_long *ot;
 RVECTOR r0,r1,r2;
 CRVECTOR3 cr[5];
} DIVPOLYGON3;
typedef struct {
 RVECTOR r01,r02,r31,r32,rc;
 RVECTOR *r0,*r1,*r2,*r3;
 u_long *rtn;
} CRVECTOR4;
typedef struct {
 u_long ndiv;
 u_long pih,piv;
 u_short clut,tpage;
 CVECTOR rgbc;
 u_long *ot;
 RVECTOR r0,r1,r2,r3;
 CRVECTOR4 cr[5];
} DIVPOLYGON4;
typedef struct {
        short xy[3];
        short uv[2];
        short rgb[3];
} SPOL;
typedef struct {
        short sxy[4][2];
        short sz[4][2];
        short uv[4][2];
        short rgb[4][3];
        short code;
} POL4;
typedef struct {
        short sxy[3][2];
        short sz[3][2];
        short uv[3][2];
        short rgb[3][3];
        short code;
} POL3;
typedef struct {
        SVECTOR *v;
        SVECTOR *n;
        SVECTOR *u;
        CVECTOR *c;
        u_long len;
} TMESH;
typedef struct {
        SVECTOR *v;
        SVECTOR *n;
        SVECTOR *u;
        CVECTOR *c;
        u_long lenv;
        u_long lenh;
} QMESH;
extern void InitGeom();
extern void EigenMatrix(MATRIX *m, MATRIX *t);
extern int IsIdMatrix(MATRIX *m);
extern MATRIX *MulMatrix0(MATRIX *m0,MATRIX *m1,MATRIX *m2);
extern MATRIX *MulRotMatrix0(MATRIX *m0,MATRIX *m1);
extern MATRIX *MulMatrix(MATRIX *m0,MATRIX *m1);
extern MATRIX *MulMatrix2(MATRIX *m0,MATRIX *m1);
extern MATRIX *MulRotMatrix(MATRIX *m0);
extern MATRIX *SetMulMatrix(MATRIX *m0,MATRIX *m1);
extern MATRIX *SetMulRotMatrix(MATRIX *m0);
extern VECTOR *ApplyMatrix(MATRIX *m,SVECTOR *v0,VECTOR *v1);
extern VECTOR *ApplyRotMatrix(SVECTOR *v0,VECTOR *v1);
extern VECTOR *ApplyRotMatrixLV(VECTOR *v0,VECTOR *v1);
extern VECTOR *ApplyMatrixLV(MATRIX *m,VECTOR *v0,VECTOR *v1);
extern SVECTOR *ApplyMatrixSV(MATRIX *m,SVECTOR *v0,SVECTOR *v1);
extern VECTOR *ApplyTransposeMatrixLV(MATRIX *m,VECTOR *v0,VECTOR *v1);
extern MATRIX *RotMatrix(SVECTOR *r,MATRIX *m);
extern MATRIX *RotMatrixXZY(SVECTOR *r,MATRIX *m);
extern MATRIX *RotMatrixYXZ(SVECTOR *r,MATRIX *m);
extern MATRIX *RotMatrixYZX(SVECTOR *r,MATRIX *m);
extern MATRIX *RotMatrixZXY(SVECTOR *r,MATRIX *m);
extern MATRIX *RotMatrixZYX(SVECTOR *r,MATRIX *m);
extern MATRIX *RotMatrix_gte(SVECTOR *r,MATRIX *m);
extern MATRIX *RotMatrixYXZ_gte(SVECTOR *r,MATRIX *m);
extern MATRIX *RotMatrixZYX_gte(SVECTOR *r,MATRIX *m);
extern MATRIX *RotMatrixX(long r,MATRIX *m);
extern MATRIX *RotMatrixY(long r,MATRIX *m);
extern MATRIX *RotMatrixZ(long r,MATRIX *m);
extern MATRIX *RotMatrixC(SVECTOR *r,MATRIX *m);
extern MATRIX *TransMatrix(MATRIX *m,VECTOR *v);
extern MATRIX *ScaleMatrix(MATRIX *m,VECTOR *v);
extern MATRIX *ScaleMatrixL(MATRIX *m,VECTOR *v);
extern MATRIX *TransposeMatrix(MATRIX *m0,MATRIX *m1);
extern MATRIX *CompMatrix(MATRIX *m0,MATRIX *m1,MATRIX *m2);
extern MATRIX *CompMatrixLV(MATRIX *m0,MATRIX *m1,MATRIX *m2);
extern void MatrixNormal(MATRIX *m, MATRIX *n);
extern void MatrixNormal_0(MATRIX *m, MATRIX *n);
extern void MatrixNormal_1(MATRIX *m, MATRIX *n);
extern void MatrixNormal_2(MATRIX *m, MATRIX *n);
extern void SetRotMatrix(MATRIX *m);
extern void SetLightMatrix(MATRIX *m);
extern void SetColorMatrix(MATRIX *m);
extern void SetTransMatrix(MATRIX *m);
extern void PushMatrix();
extern void PopMatrix();
extern void ReadRotMatrix(MATRIX *m);
extern void ReadLightMatrix(MATRIX *m);
extern void ReadColorMatrix(MATRIX *m);
extern void SetRGBcd(CVECTOR *v);
extern void SetBackColor(long rbk,long gbk,long bbk);
extern void SetFarColor(long rfc,long gfc,long bfc);
extern void SetGeomOffset(long ofx,long ofy);
extern void SetGeomScreen(long h);
extern void ReadSZfifo3(long *sz0,long *sz1,long *sz2);
extern void ReadSZfifo4(long *szx,long *sz0,long *sz1,long *sz2);
extern void ReadSXSYfifo(long *sxy0,long *sxy1,long *sxy2);
extern void ReadRGBfifo(CVECTOR *v0,CVECTOR *v1,CVECTOR *v2);
extern void ReadGeomOffset(long *ofx,long *ofy);
extern long ReadGeomScreen();
extern void TransRot_32(VECTOR *v0, VECTOR *v1, long *flag);
extern long TransRotPers(SVECTOR *v0, long *sxy, long *p, long *flag);
extern long TransRotPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0,
  long *sxy1, long *sxy2, long *p, long *flag);
extern void pers_map(int abuf, SVECTOR **vertex, int tex[4][2], u_short *dtext);
extern void PhongLine(int istart_x, int iend_x, int p, int q, u_short **pixx,
  int fs, int ft, int i4, int det);
extern long RotTransPers(SVECTOR *v0,long *sxy,long *p,long *flag);
extern long RotTransPers3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   long *sxy0,long *sxy1,long *sxy2,long *p,long *flag);
extern void RotTrans(SVECTOR *v0,VECTOR *v1,long *flag);
extern void RotTransSV(SVECTOR *v0,SVECTOR *v1,long *flag);
extern void LocalLight(SVECTOR *v0,VECTOR *v1);
extern void LightColor(VECTOR *v0,VECTOR *v1);
extern void DpqColorLight(VECTOR *v0,CVECTOR *v1,long p,CVECTOR *v2);
extern void DpqColor(CVECTOR *v0,long p,CVECTOR *v1);
extern void DpqColor3(CVECTOR *v0,CVECTOR *v1,CVECTOR *v2,
   long p,CVECTOR *v3,CVECTOR *v4,CVECTOR *v5);
extern void Intpl(VECTOR *v0,long p,CVECTOR *v1);
extern VECTOR *Square12(VECTOR *v0,VECTOR *v1);
extern VECTOR *Square0(VECTOR *v0,VECTOR *v1);
extern VECTOR *SquareSL12(SVECTOR *v0,VECTOR *v1);
extern VECTOR *SquareSL0(SVECTOR *v0,VECTOR *v1);
extern SVECTOR *SquareSS12(SVECTOR *v0,SVECTOR *v1);
extern SVECTOR *SquareSS0(SVECTOR *v0,SVECTOR *v1);
extern void NormalColor(SVECTOR *v0,CVECTOR *v1);
extern void NormalColor3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   CVECTOR *v3,CVECTOR *v4,CVECTOR *v5);
extern void NormalColorDpq(SVECTOR *v0,CVECTOR *v1,long p,CVECTOR *v2);
extern void NormalColorDpq3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,CVECTOR *v3,
   long p,CVECTOR *v4,CVECTOR *v5,CVECTOR *v6);
extern void NormalColorCol(SVECTOR *v0,CVECTOR *v1,CVECTOR *v2);
extern void NormalColorCol3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,CVECTOR *v3,
   CVECTOR *v4,CVECTOR *v5,CVECTOR *v6);
extern void ColorDpq(VECTOR *v0,CVECTOR *v1,long p,CVECTOR *v2);
extern void ColorCol(VECTOR *v0,CVECTOR *v1,CVECTOR *v2);
extern long NormalClip(long sxy0,long sxy1,long sxy2);
extern long AverageZ3(long sz0,long sz1,long sz2);
extern long AverageZ4(long sz0,long sz1,long sz2,long sz3);
extern void OuterProduct12(VECTOR *v0,VECTOR *v1,VECTOR *v2);
extern void OuterProduct0(VECTOR *v0,VECTOR *v1,VECTOR *v2);
extern long Lzc(long data);
extern long RotTransPers4(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
   long *sxy0,long *sxy1,long *sxy2,long *sxy3,
   long *p,long *flag);
extern void RotTransPersN(SVECTOR *v0,DVECTOR *v1,u_short *sz,u_short *p,
   u_short *flag,long n);
extern void RotTransPers3N(SVECTOR *v0,DVECTOR *v1,u_short *sz,u_short *flag,
   long n);
extern void RotMeshH(short *Yheight,DVECTOR *Vo,u_short *sz,u_short *flag,
   short Xoffset,short Zoffset,short m,short n,
   DVECTOR *base);
extern long RotAverage3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   long *sxy0,long *sxy1,long *sxy2,long *p,long *flag);
extern long RotAverage4(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
   long *sxy0,long *sxy1,long *sxy2,long *sxy3,
   long *p,long *flag);
extern long RotNclip3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   long *sxy0,long *sxy1,long *sxy2,long *p,long *otz,
   long *flag);
extern long RotNclip4(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
   long *sxy0,long *sxy1,long *sxy2,long *sxy3,
   long *p,long *otz,long *flag);
extern long RotAverageNclip3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   long *sxy0,long *sxy1,long *sxy2,
   long *p,long *otz,long *flag);
extern long RotAverageNclip4(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
   long *sxy0,long *sxy1,long *sxy2,long *sxy3,
   long *p,long *otz,long *flag);
extern long RotColorDpq(SVECTOR *v0,SVECTOR *v1,CVECTOR *v2,
   long *sxy,CVECTOR *v3,long *flag);
extern long RotColorDpq3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   SVECTOR *v3,SVECTOR *v4,SVECTOR *v5,CVECTOR *v6,
   long *sxy0,long *sxy1,long *sxy2,
   CVECTOR *v7,CVECTOR *v8,CVECTOR *v9,long *flag);
extern long RotAverageNclipColorDpq3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   SVECTOR *v3,SVECTOR *v4,SVECTOR *v5,CVECTOR *v6,
   long *sxy0,long *sxy1,long *sxy2,
   CVECTOR *v7,CVECTOR *v8,CVECTOR *v9,
   long *otz,long *flag);
extern long RotAverageNclipColorCol3(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   SVECTOR *v3,SVECTOR *v4,SVECTOR *v5,CVECTOR *v6,
   long *sxy0,long *sxy1,long *sxy2,
   CVECTOR *v7,CVECTOR *v8,CVECTOR *v9,
   long *otz,long *flag);
extern long RotColorMatDpq(SVECTOR *v0,SVECTOR *v1,CVECTOR *v2,long *sxy,
   CVECTOR *v3,long matc,long flag);
extern void ColorMatDpq(SVECTOR *v0,CVECTOR *v1,long p,CVECTOR *v2,long matc);
extern void ColorMatCol(SVECTOR *v0,CVECTOR *v1,CVECTOR *v2,long matc);
extern void LoadAverage12(VECTOR *v0,VECTOR *v1,long p0,long p1,VECTOR *v2);
extern void LoadAverageShort12(SVECTOR *v0,SVECTOR *v1,long p0,long p1,
   SVECTOR *v2);
extern void LoadAverage0(VECTOR *v0,VECTOR *v1,long p0,long p1,VECTOR *v2);
extern void LoadAverageShort0(SVECTOR *v0,SVECTOR *v1,long p0,long p1,
   SVECTOR *v2);
extern void LoadAverageByte(u_char *v0,u_char *v1,long p0,long p1,u_char *v2);
extern void LoadAverageCol(u_char *v0,u_char *v1,long p0,long p1,u_char *v2);
extern long VectorNormal(VECTOR *v0, VECTOR *v1);
extern long VectorNormalS(VECTOR *v0, SVECTOR *v1);
extern long VectorNormalSS(SVECTOR *v0, SVECTOR *v1);
extern long SquareRoot0(long a);
extern long SquareRoot12(long a);
extern void InvSquareRoot(long a, long *b, long *c);
extern void gteMIMefunc(SVECTOR *otp, SVECTOR *dfp, long n, long p);
extern void SetFogFar(long a,long h);
extern void SetFogNear(long a,long h);
extern void SetFogNearFar(long a,long b,long h);
extern void SubPol4(POL4 *p, SPOL *sp, int ndiv);
extern void SubPol3(POL3 *p, SPOL *sp, int ndiv);
extern int rcos(int a);
extern int rsin(int a);
extern int ccos(int a);
extern int csin(int a);
extern int cln(int a);
extern int csqrt(int a);
extern int catan(int a);
extern long ratan2(long y, long x);
extern void RotPMD_F3(long *pa,u_long *ot,int otlen,int id,int backc);
extern void RotPMD_G3(long *pa,u_long *ot,int otlen,int id,int backc);
extern void RotPMD_FT3(long *pa,u_long *ot,int otlen,int id,int backc);
extern void RotPMD_GT3(long *pa,u_long *ot,int otlen,int id,int backc);
extern void RotPMD_F4(long *pa,u_long *ot,int otlen,int id,int backc);
extern void RotPMD_G4(long *pa,u_long *ot,int otlen,int id,int backc);
extern void RotPMD_FT4(long *pa,u_long *ot,int otlen,int id,int backc);
extern void RotPMD_GT4(long *pa,u_long *ot,int otlen,int id,int backc);
extern void RotPMD_SV_F3(long *pa,long *va,u_long *ot,int otlen,int id,
   int backc);
extern void RotPMD_SV_G3(long *pa,long *va,u_long *ot,int otlen,int id,
   int backc);
extern void RotPMD_SV_FT3(long *pa,long *va,u_long *ot,int otlen,int id,
   int backc);
extern void RotPMD_SV_GT3(long *pa,long *va,u_long *ot,int otlen,int id,
   int backc);
extern void RotPMD_SV_F4(long *pa,long *va,u_long *ot,int otlen,int id,
   int backc);
extern void RotPMD_SV_G4(long *pa,long *va,u_long *ot,int otlen,int id,
   int backc);
extern void RotPMD_SV_FT4(long *pa,long *va,u_long *ot,int otlen,int id,
   int backc);
extern void RotPMD_SV_GT4(long *pa,long *va,u_long *ot,int otlen,int id,
   int backc);
extern void InitClip(EVECTOR *evbfad,long hw,long vw,long h,long near,long far);
extern long Clip3F(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,EVECTOR **evmx);
extern long Clip3FP(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,EVECTOR **evmx);
extern long Clip4F(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
   EVECTOR **evmx);
extern long Clip4FP(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
   EVECTOR **evmx);
extern long Clip3FT(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
  short *uv0,short *uv1,short *uv2,EVECTOR **evmx);
extern long Clip3FTP(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
  short *uv0,short *uv1,short *uv2,EVECTOR **evmx);
extern long Clip4FT(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
  short *uv0,short *uv1,short *uv2,short *uv3,EVECTOR **evmx);
extern long Clip4FTP(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
  short *uv0,short *uv1,short *uv2,short *uv3,EVECTOR **evmx);
extern long Clip3G(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
  CVECTOR *rgb0,CVECTOR *rgb1,CVECTOR *rgb2,EVECTOR **evmx);
extern long Clip3GP(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
  CVECTOR *rgb0,CVECTOR *rgb1,CVECTOR *rgb2,EVECTOR **evmx);
extern long Clip4G(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
  CVECTOR *rgb0,CVECTOR *rgb1,CVECTOR *rgb2,CVECTOR *rgb3,
  EVECTOR **evmx);
extern long Clip4GP(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
  CVECTOR *rgb0,CVECTOR *rgb1,CVECTOR *rgb2,CVECTOR *rgb3,
  EVECTOR **evmx);
extern long Clip3GT(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
  short *uv0,short *uv1,short *uv2,
  CVECTOR *rgb0,CVECTOR *rgb1,CVECTOR *rgb2,
  EVECTOR **evmx);
extern long Clip3GTP(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
  short *uv0,short *uv1,short *uv2,
  CVECTOR *rgb0,CVECTOR *rgb1,CVECTOR *rgb2,
  EVECTOR **evmx);
extern long Clip4GT(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
  short *uv0,short *uv1,short *uv2,short *uv3,
  CVECTOR *rgb0,CVECTOR *rgb1,CVECTOR *rgb2,CVECTOR *rgb3,
  EVECTOR **evmx);
extern long Clip4GTP(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3,
  short *uv0,short *uv1,short *uv2,short *uv3,
  CVECTOR *rgb0,CVECTOR *rgb1,CVECTOR *rgb2,CVECTOR *rgb3,
  EVECTOR **evmx);
extern void RotTransPers_nom(SVECTOR *v0);
extern void RotTransPers3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2);
extern void RotTransPers4_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *v3);
extern void RotTrans_nom(SVECTOR *v0);
extern void RotAverage3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2);
extern void RotNclip3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2);
extern void RotAverageNclip3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2);
extern void RotAverageNclipColorDpq3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   SVECTOR *v3,SVECTOR *v4,SVECTOR *v5,CVECTOR *v6);
extern void RotAverageNclipColorCol3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   SVECTOR *v3,SVECTOR *v4,SVECTOR *v5,CVECTOR *v6);
extern void RotColorDpq_nom(SVECTOR *v0,SVECTOR *v1,CVECTOR *v2);
extern long RotColorDpq3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   SVECTOR *v3,SVECTOR *v4,SVECTOR *v5,CVECTOR *v6);
extern void NormalColor_nom(SVECTOR *v0);
extern void NormalColor3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2);
extern void NormalColorDpq_nom(SVECTOR *v0,CVECTOR *v1,long p);
extern void NormalColorDpq3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   CVECTOR *v3,long p);
extern void NormalColorCol_nom(SVECTOR *v0,CVECTOR *v1);
extern void NormalColorCol3_nom(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,
   CVECTOR *v3);
extern void RotSMD_F3(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_G3(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_FT3(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_GT3(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_F4(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_G4(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_FT4(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_GT4(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_SV_F3(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_SV_G3(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_SV_FT3(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_SV_GT3(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_SV_F4(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_SV_G4(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_SV_FT4(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotSMD_SV_GT4(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_F3(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_G3(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_FT3(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_GT3(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_F4(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_G4(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_FT4(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_GT4(long *pa,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_SV_F3(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_SV_G3(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_SV_FT3(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_SV_GT3(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_SV_F4(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_SV_G4(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_SV_FT4(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern void RotRMD_SV_GT4(long *pa,long *va,u_long *ot,int otlen,int id,
   int sclip, int hclip, int vclip, int nclipmode);
extern long p2otz(long p, long projection);
extern long otz2p(long otz, long projection);
extern int (*GPU_printf)(char *fmt, ...);
typedef struct {
 short x, y;
 short w, h;
} RECT;
typedef struct {
 int x, y;
 int w, h;
} RECT32;
typedef struct {
 u_long tag;
 u_long code[15];
} DR_ENV;
typedef struct {
 RECT clip;
 short ofs[2];
 RECT tw;
 u_short tpage;
 u_char dtd;
 u_char dfe;
 u_char isbg;
 u_char r0, g0, b0;
 DR_ENV dr_env;
} DRAWENV;
typedef struct {
 RECT disp;
 RECT screen;
 u_char isinter;
 u_char isrgb24;
 u_char pad0, pad1;
} DISPENV;
typedef struct {
 unsigned addr: 24;
 unsigned len: 8;
 u_char r0, g0, b0, code;
} P_TAG;
typedef struct {
 u_char r0, g0, b0, code;
} P_CODE;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 short x1, y1;
 short x2, y2;
} POLY_F3;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 short x1, y1;
 short x2, y2;
 short x3, y3;
} POLY_F4;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char u0, v0; u_short clut;
 short x1, y1;
 u_char u1, v1; u_short tpage;
 short x2, y2;
 u_char u2, v2; u_short pad1;
} POLY_FT3;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char u0, v0; u_short clut;
 short x1, y1;
 u_char u1, v1; u_short tpage;
 short x2, y2;
 u_char u2, v2; u_short pad1;
 short x3, y3;
 u_char u3, v3; u_short pad2;
} POLY_FT4;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char r1, g1, b1, pad1;
 short x1, y1;
 u_char r2, g2, b2, pad2;
 short x2, y2;
} POLY_G3;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char r1, g1, b1, pad1;
 short x1, y1;
 u_char r2, g2, b2, pad2;
 short x2, y2;
 u_char r3, g3, b3, pad3;
 short x3, y3;
} POLY_G4;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char u0, v0; u_short clut;
 u_char r1, g1, b1, p1;
 short x1, y1;
 u_char u1, v1; u_short tpage;
 u_char r2, g2, b2, p2;
 short x2, y2;
 u_char u2, v2; u_short pad2;
} POLY_GT3;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char u0, v0; u_short clut;
 u_char r1, g1, b1, p1;
 short x1, y1;
 u_char u1, v1; u_short tpage;
 u_char r2, g2, b2, p2;
 short x2, y2;
 u_char u2, v2; u_short pad2;
 u_char r3, g3, b3, p3;
 short x3, y3;
 u_char u3, v3; u_short pad3;
} POLY_GT4;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 short x1, y1;
} LINE_F2;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char r1, g1, b1, p1;
 short x1, y1;
} LINE_G2;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 short x1, y1;
 short x2, y2;
 u_long pad;
} LINE_F3;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char r1, g1, b1, p1;
 short x1, y1;
 u_char r2, g2, b2, p2;
 short x2, y2;
 u_long pad;
} LINE_G3;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 short x1, y1;
 short x2, y2;
 short x3, y3;
 u_long pad;
} LINE_F4;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char r1, g1, b1, p1;
 short x1, y1;
 u_char r2, g2, b2, p2;
 short x2, y2;
 u_char r3, g3, b3, p3;
 short x3, y3;
 u_long pad;
} LINE_G4;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char u0, v0; u_short clut;
 short w, h;
} SPRT;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char u0, v0; u_short clut;
} SPRT_16;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 u_char u0, v0; u_short clut;
} SPRT_8;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
 short w, h;
} TILE;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
} TILE_16;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
} TILE_8;
typedef struct {
 u_long tag;
 u_char r0, g0, b0, code;
 short x0, y0;
} TILE_1;
typedef struct {
 u_long tag;
 u_long code[2];
} DR_MODE;
typedef struct {
 u_long tag;
 u_long code[2];
} DR_TWIN;
typedef struct {
 u_long tag;
 u_long code[2];
} DR_AREA;
typedef struct {
 u_long tag;
 u_long code[2];
} DR_OFFSET;
typedef struct {
 u_long tag;
 u_long code[5];
} DR_MOVE;
typedef struct {
 u_long tag;
 u_long code[3];
 u_long p[13];
} DR_LOAD;
typedef struct {
 u_long tag;
 u_long code[1];
} DR_TPAGE;
typedef struct {
        u_long tag;
        u_long code[2];
} DR_STP;
typedef struct {
 u_long id;
 u_char r0, g0, b0, p0;
 u_char r1, g1, b1, p1;
 u_char r2, g2, b2, p2;
 u_char r3, g3, b3, p3;
 u_short tpage, clut;
 u_char u0, v0, u1, v1;
 u_char u2, v2, u3, v3;
 SVECTOR x0, x1, x2, x3;
 SVECTOR n0, n1, n2, n3;
 SVECTOR *v_ofs;
 SVECTOR *n_ofs;
 u_short vert0, vert1;
 u_short vert2, vert3;
 u_short norm0, norm1;
 u_short norm2, norm3;
} TMD_PRIM;
typedef struct {
 u_long mode;
 RECT *crect;
 u_long *caddr;
 RECT *prect;
 u_long *paddr;
} TIM_IMAGE;
extern int FntPrint();
extern int KanjiFntPrint();
extern DISPENV *GetDispEnv(DISPENV *env);
extern DISPENV *PutDispEnv(DISPENV *env);
extern DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h);
extern DRAWENV *GetDrawEnv(DRAWENV *env);
extern DRAWENV *PutDrawEnv(DRAWENV *env);
extern DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h);
extern TIM_IMAGE *ReadTIM(TIM_IMAGE *timimg);
extern TMD_PRIM *ReadTMD(TMD_PRIM *tmdprim);
extern int CheckPrim(char *s, u_long *p);
extern int ClearImage(RECT *rect, u_char r, u_char g, u_char b);
extern int ClearImage2(RECT *rect, u_char r, u_char g, u_char b);
extern int DrawSync(int mode);
extern int FntOpen(int x, int y, int w, int h, int isbg, int n);
extern int GetGraphDebug(void) ;
extern int GetTimSize(u_char *sjis);
extern int IsEndPrim(void *p) ;
extern int KanjiFntOpen(int x, int y, int w, int h, int dx, int dy, int cx, int cy, int isbg, int n);
extern void KanjiFntClose(void);
extern int Krom2Tim(u_char *sjis, u_long *taddr, int dx, int dy, int cdx, int cdy, u_int fg, u_int bg);
extern int LoadImage(RECT *rect, u_long *p);
extern int MargePrim(void *p0, void *p1);
extern int MoveImage(RECT *rect, int x, int y);
extern int OpenTIM(u_long *addr);
extern int OpenTMD(u_long *tmd, int obj_no);
extern int ResetGraph(int mode);
extern int SetGraphDebug(int level);
extern int StoreImage(RECT *rect, u_long *p);
extern u_long *ClearOTag(u_long *ot, int n);
extern u_long *ClearOTagR(u_long *ot, int n);
extern u_long *FntFlush(int id);
extern u_long *KanjiFntFlush(int id);
extern u_long DrawSyncCallback(void (*func)(void));
extern u_short GetClut(int x, int y) ;
extern u_short GetTPage(int tp, int abr, int x, int y) ;
extern u_short LoadClut(u_long *clut, int x, int y);
extern u_short LoadClut2(u_long *clut, int x, int y);
extern u_short LoadTPage(u_long *pix, int tp, int abr, int x, int y, int w, int h);
extern void *NextPrim(void *p) ;
extern void AddPrim(void *ot, void *p) ;
extern void AddPrims(void *ot, void *p0, void *p1) ;
extern void CatPrim(void *p0, void *p1) ;
extern void DrawOTag(u_long *p);
extern void DrawOTagIO(u_long *p);
extern void DrawOTagEnv(u_long *p, DRAWENV *env);
extern void DrawPrim(void *p);
extern void DumpClut(u_short clut) ;
extern void DumpDispEnv(DISPENV *env);
extern void DumpDrawEnv(DRAWENV *env);
extern void DumpOTag(u_long *p);
extern void DumpTPage(u_short tpage) ;
extern void FntLoad(int tx, int ty);
extern void SetDispMask(int mask);
extern void SetDrawArea(DR_AREA *p, RECT *r);
extern void SetDrawEnv(DR_ENV *dr_env, DRAWENV *env);
extern void SetDrawLoad(DR_LOAD *p, RECT *rect);
extern void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw);
extern void SetDrawTPage(DR_TPAGE *p, int dfe, int dtd, int tpage);
extern void SetDrawMove(DR_MOVE *p, RECT *rect, int x, int y) ;
extern void SetDrawOffset(DR_OFFSET *p, u_short *ofs);
extern void SetDrawStp(DR_STP *p, int pbw);
extern void SetDumpFnt(int id);
extern void SetLineF2(LINE_F2 *p) ;
extern void SetLineF3(LINE_F3 *p) ;
extern void SetLineF4(LINE_F4 *p) ;
extern void SetLineG2(LINE_G2 *p) ;
extern void SetLineG3(LINE_G3 *p) ;
extern void SetLineG4(LINE_G4 *p) ;
extern void SetPolyF3(POLY_F3 *p) ;
extern void SetPolyF4(POLY_F4 *p) ;
extern void SetPolyFT3(POLY_FT3 *p) ;
extern void SetPolyFT4(POLY_FT4 *p) ;
extern void SetPolyG3(POLY_G3 *p) ;
extern void SetPolyG4(POLY_G4 *p) ;
extern void SetPolyGT3(POLY_GT3 *p) ;
extern void SetPolyGT4(POLY_GT4 *p) ;
extern void SetSemiTrans(void *p, int abe) ;
extern void SetShadeTex(void *p, int tge) ;
extern void SetSprt(SPRT *p) ;
extern void SetSprt16(SPRT_16 *p) ;
extern void SetSprt8(SPRT_8 *p) ;
extern void SetTexWindow(DR_TWIN *p, RECT *tw);
extern void SetTile(TILE *p) ;
extern void SetTile1(TILE_1 *p) ;
extern void SetTile16(TILE_16 *p) ;
extern void SetTile8(TILE_8 *p) ;
extern void TermPrim(void *p) ;
extern u_long *BreakDraw(void);
extern void ContinueDraw(u_long *insaddr, u_long *contaddr);
extern int IsIdleGPU(int max_count);
extern int GetODE(void);
extern int LoadImage2(RECT *rect, u_long *p);
extern int StoreImage2(RECT *rect, u_long *p);
extern int MoveImage2(RECT *rect, int x, int y);
extern int DrawOTag2(u_long *p);
extern void GetDrawMode(DR_MODE *p);
extern void GetTexWindow(DR_TWIN *p);
extern void GetDrawArea(DR_AREA *p);
extern void GetDrawOffset(DR_OFFSET *p);
extern void GetDrawEnv2(DR_ENV *p);
typedef void (*CdlCB)(u_char,u_char *);
typedef struct {
 u_char minute;
 u_char second;
 u_char sector;
 u_char track;
} CdlLOC;
typedef struct {
 u_char file;
 u_char chan;
 u_short pad;
} CdlFILTER;
typedef struct {
 u_char val0;
 u_char val1;
 u_char val2;
 u_char val3;
} CdlATV;
typedef struct {
 CdlLOC pos;
 u_long size;
 char name[16];
} CdlFILE;
typedef struct {
    u_short id;
    u_short type;
    u_short secCount;
    u_short nSectors;
    u_long frameCount;
    u_long frameSize;
    u_short width;
    u_short height;
    u_long dummy1;
    u_long dummy2;
    CdlLOC loc;
} StHEADER;
void StSetRing(u_long *ring_addr,u_long ring_size);
void StClearRing(void);
void StUnSetRing(void);
void StSetStream(u_long mode,u_long start_frame,u_long end_frame,
      void (*func1)(),void (*func2)());
void StSetEmulate(u_long *addr,u_long mode,u_long start_frame,
       u_long end_frame,void (*func1)(),void (*func2)());
u_long StFreeRing(u_long *base);
u_long StGetNext(u_long **addr,u_long **header);
u_long StGetNextS(u_long **addr,u_long **header);
u_short StNextStatus(u_long **addr,u_long **header);
void StRingStatus(short *free_sectors,short *over_sectors);
void StSetMask(u_long mask,u_long start,u_long end);
void StCdInterrupt(void);
int StGetBackloc(CdlLOC *loc);
int StSetChannel(u_long channel);
void CdFlush(void);
CdlFILE *CdSearchFile(CdlFILE *fp, char *name);
CdlLOC *CdIntToPos(int i, CdlLOC *p) ;
char *CdComstr(u_char com);
char *CdIntstr(u_char intr);
int CdControl(u_char com, u_char *param, u_char *result);
int CdControlB(u_char com, u_char *param, u_char *result);
int CdControlF(u_char com, u_char *param);
int CdGetSector(void *madr, int size);
int CdGetSector2( void* madr, int size );
int CdDataSync(int mode);
int CdGetToc(CdlLOC *loc) ;
int CdPlay(int mode, int *track, int offset);
int CdMix(CdlATV *vol);
int CdPosToInt(CdlLOC *p);
int CdRead(int sectors, u_long *buf, int mode);
int CdRead2(long mode);
int CdReadFile(char *file, u_long *addr, int nbyte);
int CdReadSync(int mode, u_char *result);
int CdReady(int mode, u_char *result) ;
int CdSetDebug(int level);
int CdSync(int mode, u_char *result) ;
void (*CdDataCallback(void (*func)()));
CdlCB CdReadCallback(CdlCB func);
CdlCB CdReadyCallback(CdlCB func);
CdlCB CdSyncCallback(CdlCB func);
int CdInit(void);
int CdReset(int mode);
int CdStatus(void);
int CdLastCom(void);
CdlLOC *CdLastPos(void);
int CdMode(void);
int CdDiskReady( int mode );
int CdGetDiskType( void );
struct EXEC *CdReadExec(char *file);
void CdReadBreak( void );
typedef struct {
    short left;
    short right;
} SpuVolume;
typedef struct {
    unsigned long voice;
    unsigned long mask;
    SpuVolume volume;
    SpuVolume volmode;
    SpuVolume volumex;
    unsigned short pitch;
    unsigned short note;
    unsigned short sample_note;
    short envx;
    unsigned long addr;
    unsigned long loop_addr;
    long a_mode;
    long s_mode;
    long r_mode;
    unsigned short ar;
    unsigned short dr;
    unsigned short sr;
    unsigned short rr;
    unsigned short sl;
    unsigned short adsr1;
    unsigned short adsr2;
} SpuVoiceAttr;
typedef struct {
    short voiceNum;
    short pad;
    SpuVoiceAttr attr;
} SpuLVoiceAttr;
typedef struct {
    unsigned long mask;
    long mode;
    SpuVolume depth;
    long delay;
    long feedback;
} SpuReverbAttr;
typedef struct {
    short cd_left [0x200];
    short cd_right [0x200];
    short voice1 [0x200];
    short voice3 [0x200];
} SpuDecodedData;
typedef SpuDecodedData SpuDecodeData;
typedef struct {
    SpuVolume volume;
    long reverb;
    long mix;
} SpuExtAttr;
typedef struct {
    unsigned long mask;
    SpuVolume mvol;
    SpuVolume mvolmode;
    SpuVolume mvolx;
    SpuExtAttr cd;
    SpuExtAttr ext;
} SpuCommonAttr;
typedef void (*SpuIRQCallbackProc)(void);
typedef void (*SpuTransferCallbackProc)(void);
typedef struct {
    unsigned long mask;
    unsigned long queueing;
} SpuEnv;
extern void SpuInit (void);
extern void SpuInitHot (void);
extern void SpuStart (void);
extern void SpuQuit (void);
extern long SpuSetMute (long on_off);
extern long SpuGetMute (void);
extern void SpuSetEnv (SpuEnv *env);
extern long SpuSetNoiseClock (long n_clock);
extern long SpuGetNoiseClock (void);
extern unsigned long SpuSetNoiseVoice (long on_off, unsigned long voice_bit);
extern unsigned long SpuGetNoiseVoice (void);
extern long SpuSetReverb (long on_off);
extern long SpuGetReverb (void);
extern long SpuSetReverbModeParam (SpuReverbAttr *attr);
extern void SpuGetReverbModeParam (SpuReverbAttr *attr);
extern long SpuSetReverbDepth (SpuReverbAttr *attr);
extern long SpuReserveReverbWorkArea (long on_off);
extern long SpuIsReverbWorkAreaReserved (long on_off);
extern unsigned long SpuSetReverbVoice (long on_off, unsigned long voice_bit);
extern unsigned long SpuGetReverbVoice (void);
extern long SpuClearReverbWorkArea (long mode);
extern unsigned long SpuWrite (unsigned char *addr, unsigned long size);
extern unsigned long SpuWrite0 (unsigned long size);
extern unsigned long SpuRead (unsigned char *addr, unsigned long size);
extern long SpuSetTransferMode (long mode);
extern long SpuGetTransferMode (void);
extern unsigned long SpuSetTransferStartAddr (unsigned long addr);
extern unsigned long SpuGetTransferStartAddr (void);
extern unsigned long SpuWritePartly (unsigned char *addr, unsigned long size);
extern long SpuIsTransferCompleted (long flag);
extern SpuTransferCallbackProc SpuSetTransferCallback (SpuTransferCallbackProc func);
extern long SpuReadDecodedData (SpuDecodedData *d_data, long flag);
extern long SpuSetIRQ (long on_off);
extern long SpuGetIRQ (void);
extern unsigned long SpuSetIRQAddr (unsigned long);
extern unsigned long SpuGetIRQAddr (void);
extern SpuIRQCallbackProc SpuSetIRQCallback (SpuIRQCallbackProc);
extern void SpuSetVoiceAttr (SpuVoiceAttr *arg);
extern void SpuGetVoiceAttr (SpuVoiceAttr *arg);
extern void SpuSetKey (long on_off, unsigned long voice_bit);
extern void SpuSetKeyOnWithAttr (SpuVoiceAttr *attr);
extern long SpuGetKeyStatus (unsigned long voice_bit);
extern void SpuGetAllKeysStatus (char *status);
extern unsigned long SpuFlush (unsigned long ev);
extern unsigned long SpuSetPitchLFOVoice (long on_off, unsigned long voice_bit);
extern unsigned long SpuGetPitchLFOVoice (void);
extern void SpuSetCommonAttr (SpuCommonAttr *attr);
extern void SpuGetCommonAttr (SpuCommonAttr *attr);
extern long SpuInitMalloc (long num, char *top);
extern long SpuMalloc (long size);
extern long SpuMallocWithStartAddr (unsigned long addr, long size);
extern void SpuFree (unsigned long addr);
extern long SpuRGetAllKeysStatus (long min_, long max_, char *status);
extern long SpuRSetVoiceAttr (long min_, long max_, SpuVoiceAttr *arg);
extern void SpuNSetVoiceAttr (int vNum, SpuVoiceAttr *arg);
extern void SpuNGetVoiceAttr (int vNum, SpuVoiceAttr *arg);
extern void SpuLSetVoiceAttr (int num, SpuLVoiceAttr *argList);
extern void SpuSetVoiceVolume (int vNum, short volL, short volR);
extern void SpuSetVoiceVolumeAttr (int vNum, short volL, short volR,
       short volModeL, short volModeR);
extern void SpuSetVoicePitch (int vNum, unsigned short pitch);
extern void SpuSetVoiceNote (int vNum, unsigned short note);
extern void SpuSetVoiceSampleNote (int vNum, unsigned short sampleNote);
extern void SpuSetVoiceStartAddr (int vNum, unsigned long startAddr);
extern void SpuSetVoiceLoopStartAddr (int vNum, unsigned long lsa);
extern void SpuSetVoiceAR (int vNum, unsigned short AR);
extern void SpuSetVoiceDR (int vNum, unsigned short DR);
extern void SpuSetVoiceSR (int vNum, unsigned short SR);
extern void SpuSetVoiceRR (int vNum, unsigned short RR);
extern void SpuSetVoiceSL (int vNum, unsigned short SL);
extern void SpuSetVoiceARAttr (int vNum, unsigned short AR, long ARmode);
extern void SpuSetVoiceSRAttr (int vNum, unsigned short SR, long SRmode);
extern void SpuSetVoiceRRAttr (int vNum, unsigned short RR, long RRmode);
extern void SpuSetVoiceADSR (int vNum, unsigned short AR, unsigned short DR,
        unsigned short SR, unsigned short RR,
        unsigned short SL);
extern void SpuSetVoiceADSRAttr (int vNum,
     unsigned short AR, unsigned short DR,
     unsigned short SR, unsigned short RR,
     unsigned short SL,
     long ARmode, long SRmode, long RRmode);
extern void SpuGetVoiceVolume (int vNum, short *volL, short *volR);
extern void SpuGetVoiceVolumeAttr (int vNum, short *volL, short *volR,
       short *volModeL, short *volModeR);
extern void SpuGetVoiceVolumeX (int vNum, short *volXL, short *volXR);
extern void SpuGetVoicePitch (int vNum, unsigned short *pitch);
extern void SpuGetVoiceNote (int vNum, unsigned short *note);
extern void SpuGetVoiceSampleNote (int vNum, unsigned short *sampleNote);
extern void SpuGetVoiceEnvelope (int vNum, short *envx);
extern void SpuGetVoiceStartAddr (int vNum, unsigned long *startAddr);
extern void SpuGetVoiceLoopStartAddr (int vNum, unsigned long *loopStartAddr);
extern void SpuGetVoiceAR (int vNum, unsigned short *AR);
extern void SpuGetVoiceDR (int vNum, unsigned short *DR);
extern void SpuGetVoiceSR (int vNum, unsigned short *SR);
extern void SpuGetVoiceRR (int vNum, unsigned short *RR);
extern void SpuGetVoiceSL (int vNum, unsigned short *SL);
extern void SpuGetVoiceARAttr (int vNum, unsigned short *AR, long *ARmode);
extern void SpuGetVoiceSRAttr (int vNum, unsigned short *SR, long *SRmode);
extern void SpuGetVoiceRRAttr (int vNum, unsigned short *RR, long *RRmode);
extern void SpuGetVoiceADSR (int vNum,
        unsigned short *AR, unsigned short *DR,
        unsigned short *SR, unsigned short *RR,
        unsigned short *SL);
extern void SpuGetVoiceADSRAttr (int vNum,
     unsigned short *AR, unsigned short *DR,
     unsigned short *SR, unsigned short *RR,
     unsigned short *SL,
     long *ARmode, long *SRmode, long *RRmode);
extern void SpuGetVoiceEnvelopeAttr (int vNum, long *keyStat, short *envx );
extern void SpuSetCommonMasterVolume (short mvol_left, short mvol_right);
extern void SpuSetCommonMasterVolumeAttr (short mvol_left, short mvol_right,
       short mvolmode_left,
       short mvolmode_right);
extern void SpuSetCommonCDMix (long cd_mix);
extern void SpuSetCommonCDVolume (short cd_left, short cd_right);
extern void SpuSetCommonCDReverb (long cd_reverb);
extern void SpuGetCommonMasterVolume (short *mvol_left, short *mvol_right);
extern void SpuGetCommonMasterVolumeX (short *mvolx_left, short *mvolx_right);
extern void SpuGetCommonMasterVolumeAttr (short *mvol_left, short *mvol_right,
       short *mvolmode_left,
       short *mvolmode_right);
extern void SpuGetCommonCDMix (long *cd_mix);
extern void SpuGetCommonCDVolume (short *cd_left, short *cd_right);
extern void SpuGetCommonCDReverb (long *cd_reverb);
extern long SpuSetReverbModeType (long mode);
extern void SpuSetReverbModeDepth (short depth_left, short depth_right);
extern void SpuSetReverbModeDelayTime (long delay);
extern void SpuSetReverbModeFeedback (long feedback);
extern void SpuGetReverbModeType (long *mode);
extern void SpuGetReverbModeDepth (short *depth_left, short *depth_right);
extern void SpuGetReverbModeDelayTime (long *delay);
extern void SpuGetReverbModeFeedback (long *feedback);
extern void SpuSetESA( long revAddr );
typedef struct {
    char status;
    char pad1;
    char pad2;
    char pad3;
    long last_size;
    unsigned long buf_addr;
    unsigned long data_addr;
} SpuStVoiceAttr;
typedef struct {
    long size;
    long low_priority;
    SpuStVoiceAttr voice [24];
} SpuStEnv;
typedef void (*SpuStCallbackProc)(unsigned long, long);
extern SpuStEnv *SpuStInit (long);
extern long SpuStQuit (void);
extern long SpuStGetStatus (void);
extern unsigned long SpuStGetVoiceStatus (void);
extern long SpuStTransfer (long flag, unsigned long voice_bit);
extern SpuStCallbackProc SpuStSetPreparationFinishedCallback (SpuStCallbackProc func);
extern SpuStCallbackProc SpuStSetTransferFinishedCallback (SpuStCallbackProc func);
extern SpuStCallbackProc SpuStSetStreamFinishedCallback (SpuStCallbackProc func);
extern int PadIdentifier;
int CheckCallback(void) ;
void PadInit(int mode);
int ResetCallback(void) ;
int RestartCallback(void) ;
int StopCallback(void) ;
int VSync(int mode);
int VSyncCallback(void (*f)(void)) ;
long GetVideoMode (void);
long SetVideoMode (long mode);
u_long PadRead(int id);
void PadStop(void);
void PadInitDirect(unsigned char *, unsigned char *);
void PadInitMtap(unsigned char *, unsigned char *);
void PadInitGun(unsigned char *, int);
int PadChkVsync(void);
void PadStartCom(void);
void PadStopCom(void);
unsigned PadEnableCom(unsigned);
void PadEnableGun(unsigned char);
void PadRemoveGun(void);
int PadGetState(int);
int PadInfoMode(int, int, int);
int PadInfoAct(int, int, int);
int PadInfoComb(int, int, int);
int PadSetActAlign(int, unsigned char *);
int PadSetMainMode(int socket, int offs, int lock);
void PadSetAct(int, unsigned char *, int);
__asm__(".include \"include/labels.inc\"\n");
