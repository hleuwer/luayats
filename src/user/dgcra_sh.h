#include "in1out.h"

class	dgcra_s:	public	in1out {
typedef	in1out	baseclass;

public:	
	void	init(void);
	rec_typ	REC(data *, int);
	int	command(char *, tok_typ *);
	void	restim(void);
	void	early(event *);
	int	export(exp_typ *);
	dat_typ	inp_type;

private:
	double	TAT_s , TAT_p;
	int	delta_time, delta_time_p, delta_time_s;
	double	Limit_p, Limit_s;
	double	Inc_p, Inc_s;
	double	Inc_p_error, Inc_s_error;
	double	Inc_p_real, Inc_s_real;
	int	q_max;
	int	q_len;
	data	*q_first;
	data	*q_last;
};
	
