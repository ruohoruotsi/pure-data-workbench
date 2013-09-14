/*
	randomwalk.c
	3.16.2008 copyleft greg surges
	takes 3 arguments: low-bound, high-bound, and step width
	takes bang messages and returns values from a random walk
*/

#include "m_pd.h"
#include "stdlib.h" // for rand()

static t_class *randomwalk_class; // defines a pointer to the object

typedef struct _randomwalk 
{
	t_object x_obj; // the required t_object
	t_float current; // the current value
	t_float step;	// step width
	t_float lower, upper;	// bounds
	t_outlet *f_out; // outlet pointer
} t_randomwalk;


void *randomwalk_new(t_symbol *s, int argc, t_atom *argv)
/* 3 arguments: lowbound, highbound, width */
{
	t_randomwalk *x = (t_randomwalk *)pd_new(randomwalk_class); // x points to new randomwalk obj
	t_float lowbound = 0, highbound = 0; // initialize both bounds
	
	x->step = 1; // set default step width to 1
	
	switch(argc)
	{
	default:
	case 3:
		x->step = atom_getfloat(argv + 2);
	case 2:
		highbound = atom_getfloat(argv + 1);
	case 1:
		lowbound = atom_getfloat(argv);
		break;
	case 0:
		;
	}

	if(argc < 2) // in case of only two arguments, set low-bound to 0
	{
		highbound = lowbound;
		lowbound = 0;
	}

	if(highbound > lowbound) // make sure that low-bound is actually lower than high-bound...
	{
		x->lower = lowbound;
		x->upper = highbound;
	}
	else					 // ...and swap the values if not
	{
		x->lower = highbound;
		x->upper = lowbound;
	}
	
	x->current = x->lower; // initialize the current value to the low-bound, should 
						   // this initialize to the mid-point instead?
	
	floatinlet_new(&x->x_obj, &x->lower); // inlets for dynamic bound and step values
	floatinlet_new(&x->x_obj, &x->upper);
	floatinlet_new(&x->x_obj, &x->step);

	
	x->f_out = outlet_new(&x->x_obj, &s_float); // output the float value
	
	return (void *) x;
}

void randomwalk_bang(t_randomwalk *x) // the randomwalk algorithm
{
	t_float randval = rand() % 2; 
	if(randval == 0) randval = -1;
	if(randval == 1) randval = 1;
	t_float next = x->current + (randval * 	x->step); // get the possible next value
	if(next > x->upper) next = x->upper; // check if within range
	if(next < x->lower) next = x->lower; // and if not, set it to the closest boundary value
	x->current = next; // set current to the (now constrained) next value
	outlet_float(x->f_out, x->current); 
}

void randomwalk_setup()
{
	randomwalk_class = class_new(gensym("randomwalk"),
								(t_newmethod)randomwalk_new,
								0,
								sizeof(t_randomwalk),
								CLASS_DEFAULT,
								A_GIMME,
								0);
	class_addbang(randomwalk_class, randomwalk_bang);
}
								