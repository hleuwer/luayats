//tolua_begin
class dummyObj: public in1out {
typedef	in1out	baseclass;
public:
	dummyObj();
	~dummyObj();
//tolua_end
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
};  //tolua_export
