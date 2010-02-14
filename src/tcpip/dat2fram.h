
//tolua_begin
class dat2fram: public in1out
{
   typedef in1out baseclass;
public:
   dat2fram();
   ~dat2fram(); 	

   int flen;
   int connID;
   char srcMac[6];
   char dstMac[6];
   unsigned int smac;
   unsigned int dmac;
   int tpid;
   int vlanid;
   int vlanprio;
   int pcp;
   int dscp;
   int tpid2;
   int vlanid2;
   int vlanprio2;
//tolua_end

   rec_typ REC(data *, int); // REC is a macro normally expanding to rec (for debugging)
};  //tolua_export
