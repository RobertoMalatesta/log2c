/****************************************************************/
/* Copyright (c) 1998 Luc Van Oostenryck. All rights reserved.	*/
/*								*/
/****************************************************************/

#ifndef	_ATOMS_H_
#define _ATOMS_H_

#define ATOM_and 		 ATOM___2F_5C
#define ATOM_ar_equals 		 ATOM___3D_3A_3D
#define ATOM_ar_larger 		 ATOM___3E
#define ATOM_ar_larger_equal 	 ATOM___3E_3D
#define ATOM_ar_not_equal 	 ATOM___3D_5C_3D
#define ATOM_ar_smaller 	 ATOM___3C
#define ATOM_ar_smaller_equal 	 ATOM___3D_3C
#define ATOM_at_equals 		 ATOM___3D_40_3D
#define ATOM_at_larger 		 ATOM___40_3E
#define ATOM_at_larger_equal 	 ATOM___40_3E_3D
#define ATOM_at_not_equals 	 ATOM___5C_3D_40_3D
#define ATOM_at_smaller 	 ATOM___40_3C
#define ATOM_at_smaller_equal 	 ATOM___40_3D_3C
#define ATOM_backslash 		 ATOM___5C
#define ATOM_bar 		 ATOM___7C
#define ATOM_comma 		 ATOM___2C
#define ATOM_curl 		 ATOM___7B_7D
#define ATOM_div 		 ATOM___2F_2F
#define ATOM_divide 		 ATOM___2F
#define ATOM_dot 		 ATOM___2E
#define ATOM_doublestar 	 ATOM___2A_2A
#define ATOM_grammar 		 ATOM___2D_2D_3E
#define ATOM_hat 		 ATOM___5E
#define ATOM_ifthen 		 ATOM___2D_3E
#define ATOM_isovar 		 ATOM___24VAR
#define ATOM_lshift 		 ATOM___3C_3C
#define ATOM_minus 		 ATOM___2D
#define ATOM_module 		 ATOM___3A
#define ATOM_nil 		 ATOM___5B_5D
#define ATOM_not_provable 	 ATOM___5C_2B
#define ATOM_not_strick_equals 	 ATOM___5C_3D_3D
#define ATOM_not_unifiable 	 ATOM___5C_3D
#define ATOM_obtain 		 ATOM___3F
#define ATOM_or 		 ATOM___5C_2F
#define ATOM_plus 		 ATOM___2B
#define ATOM_prove 		 ATOM___3A_2D
#define ATOM_query 		 ATOM___3F_2D
#define ATOM_rshift 		 ATOM___3E_3E
#define ATOM_semicolon 		 ATOM___3B
#define ATOM_softcut 		 ATOM___2A_2D_3E
#define ATOM_star 		 ATOM___2A
#define ATOM_str_pos 		 ATOM___24stream__position
#define ATOM_strick_equals 	 ATOM___3D_3D
#define ATOM_unify 		 ATOM___3D
#define ATOM_univ 		 ATOM___3D_2E_2E

extern atom__t ATOM__;
extern atom__t ATOM___25g;
extern atom__t ATOM___2B_3E;
extern atom__t ATOM___3A_2B;
extern atom__t ATOM__alias;
extern atom__t ATOM__append;
extern atom__t ATOM__at;
extern atom__t ATOM__binary;
extern atom__t ATOM__cpu__time;
extern atom__t ATOM__end__of__file;
extern atom__t ATOM__eof__action;
extern atom__t ATOM__eof__code;
extern atom__t ATOM__error;
extern atom__t ATOM__execute;
extern atom__t ATOM__exist;
extern atom__t ATOM__fail;
extern atom__t ATOM__false;
extern atom__t ATOM__file__name;
extern atom__t ATOM__fx;
extern atom__t ATOM__fy;
extern atom__t ATOM__heap__stack;
extern atom__t ATOM__input;
extern atom__t ATOM__is;
extern atom__t ATOM__local__stack;
extern atom__t ATOM__mark;
extern atom__t ATOM__mod;
extern atom__t ATOM__module__transparent;
extern atom__t ATOM__none;
extern atom__t ATOM__not;
extern atom__t ATOM__off;
extern atom__t ATOM__on;
extern atom__t ATOM__output;
extern atom__t ATOM__past;
extern atom__t ATOM__pipe;
extern atom__t ATOM__quiet;
extern atom__t ATOM__read;
extern atom__t ATOM__real__time;
extern atom__t ATOM__rem;
extern atom__t ATOM__reposition;
extern atom__t ATOM__reset;
extern atom__t ATOM__runtime;
extern atom__t ATOM__sheap__stack;
extern atom__t ATOM__singletons;
extern atom__t ATOM__stderr;
extern atom__t ATOM__subterm__positions;
extern atom__t ATOM__syntax__errors;
extern atom__t ATOM__system;
extern atom__t ATOM__system__time;
extern atom__t ATOM__term__positions;
extern atom__t ATOM__text;
extern atom__t ATOM__trail__stack;
extern atom__t ATOM__true;
extern atom__t ATOM__type;
extern atom__t ATOM__update;
extern atom__t ATOM__user;
extern atom__t ATOM__user__error;
extern atom__t ATOM__user__input;
extern atom__t ATOM__user__output;
extern atom__t ATOM__user__time;
extern atom__t ATOM__variable__names;
extern atom__t ATOM__write;
extern atom__t ATOM__xf;
extern atom__t ATOM__xfx;
extern atom__t ATOM__xfy;
extern atom__t ATOM__xor;
extern atom__t ATOM__yf;
extern atom__t ATOM__yfx;
extern atom__t ATOM__yfy;
extern atom__t ATOM_and;
extern atom__t ATOM_ar_equals;
extern atom__t ATOM_ar_larger;
extern atom__t ATOM_ar_larger_equal;
extern atom__t ATOM_ar_not_equal;
extern atom__t ATOM_ar_smaller;
extern atom__t ATOM_ar_smaller_equal;
extern atom__t ATOM_at_equals;
extern atom__t ATOM_at_larger;
extern atom__t ATOM_at_larger_equal;
extern atom__t ATOM_at_not_equals;
extern atom__t ATOM_at_smaller;
extern atom__t ATOM_at_smaller_equal;
extern atom__t ATOM_backslash;
extern atom__t ATOM_bar;
extern atom__t ATOM_comma;
extern atom__t ATOM_curl;
extern atom__t ATOM_div;
extern atom__t ATOM_divide;
extern atom__t ATOM_dot;
extern atom__t ATOM_doublestar;
extern atom__t ATOM_grammar;
extern atom__t ATOM_hat;
extern atom__t ATOM_ifthen;
extern atom__t ATOM_isovar;
extern atom__t ATOM_lshift;
extern atom__t ATOM_minus;
extern atom__t ATOM_module;
extern atom__t ATOM_nil;
extern atom__t ATOM_not_provable;
extern atom__t ATOM_not_strick_equals;
extern atom__t ATOM_not_unifiable;
extern atom__t ATOM_or;
extern atom__t ATOM_plus;
extern atom__t ATOM_prove;
extern atom__t ATOM_rshift;
extern atom__t ATOM_semicolon;
extern atom__t ATOM_softcut;
extern atom__t ATOM_star;
extern atom__t ATOM_strick_equals;
extern atom__t ATOM_unify;
extern atom__t ATOM_univ;



#define FUN_curl_1		 FUN___7B_7D_1
#define FUN_comma_2 		 FUN___2C_2
#define FUN_div_2 		 FUN___2F_2F_2
#define FUN_divide_2 		 FUN___2F_2
#define FUN_dot_2 		 FUN___2E_2
#define FUN_isovar_1 		 FUN___24VAR_2
#define FUN_minus_1 		 FUN___2D_1
#define FUN_minus_2 		 FUN___2D_2
#define FUN_module_2 		 FUN___3A_2
#define FUN_plus_2 		 FUN___2B_2
#define FUN_star_2 		 FUN___2A_2
#define FUN_str_pos_3 		 FUN___24stream__position_3
#define FUN_unify_2 		 FUN___3D_2

extern fun__t FUN__alias_1;
extern fun__t FUN__dt_6;
extern fun__t FUN__eof__action_1;
extern fun__t FUN__end__of__stream_1;
extern fun__t FUN__file__name_1;
extern fun__t FUN__ip_4;
extern fun__t FUN__max_2;
extern fun__t FUN__min_2;
extern fun__t FUN__mod_2;
extern fun__t FUN__mode_1;
extern fun__t FUN__pipe_1;
extern fun__t FUN__position_1;
extern fun__t FUN__reposition_1;
extern fun__t FUN__type_1;
extern fun__t FUN_comma_2;
extern fun__t FUN_curl_1;
extern fun__t FUN_div_2;
extern fun__t FUN_divide_2;
extern fun__t FUN_dot_2;
extern fun__t FUN_isovar_1;
extern fun__t FUN_minus_1;
extern fun__t FUN_minus_2;
extern fun__t FUN_module_2;
extern fun__t FUN_plus_2;
extern fun__t FUN_star_2;
extern fun__t FUN_str_pos_3;
extern fun__t FUN_unify_2;


#endif 	// _ATOMS_H_