/************************************************************************/
/* Copyright (c) 1998-2014 Luc Van Oostenryck. All rights reserved.	*/
/*									*/
/* %@%LICENSE								*/
/*									*/
/************************************************************************/

:- module(comp, [
		comp_file/1
	]).

:- use_module([ vars, builtin, trad, atoms, map_name, hash]).
:- use_module([modules, trans]).
:- use_module(code).
:- use_module(util).
:- use_module(errmsg).
:- use_module(foreign).
:- use_module(labels).
:- use_module(mapli).
:- use_module(input).
:- use_module(export).

:- op(1200, xfx, :+).
:- op(900, fy, +>).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

comp_file(File) :-
	file_type(File, Name, Mod, S),
	read_module(S, L),
	flag(current_module, _, Mod),
	(
		Mod==system
	->
		Li = L
	;
		concat($, _, Mod)
	->
		Li = L
	;
		Li = [ (:-use_module(system))|L]
	),

	open_files(Name, _Fc, Fm, _Fh),
	format(mod, 'module(~q,\n', [Mod]),
	set_output(c),
	code_module(Mod, Li, Lo, []), !,
	trad(Lo),
	nl,
	init_hash_jmps(Mod),
	close(mod),
	set_output(user_output),
	close(c),
	(
		Mod == user
	->
		link_file(Name)
	;
		close_h,
		error_report
	->
		delete_file(Fm),
		halt(1)
	;
		true
	).

open_files(Name, C, H, M) :-
	module_filename(c, Name, C),
	module_filename(mod, Name, M),
	module_filename(h, Name, H),
	open(C, write, _, [alias(c)]),
	open(M, write, _, [alias(mod)]),
	open(H, write, _, [alias(h)]).

close_h :-
	format(h, '~n#endif~n', []),
	close(h).

link_file(Name) :-
	flag(input_file, _, Name),
	module_filename('lnk.c', Name, File_Lnk),
	open(File_Lnk, write, _, [alias(lnk)]),
	set_output(lnk),
	format('#include <Prolog.h>\n#include <pl-trad.h>\n\n'),
	code_anf(Name),
	init_hash_mods(Name),
	set_output(user_output),
	close_h,
	close(lnk), !,
	(
		error_report
	->
		fail
	;
		
		link(Name)
	->
		true
	;
		fail
	).

code_anf(N) :-
	read_mods(N, A, F, P),
	findall(V, recorded(module_export, module_export(user, V)), Puser),
	append(P, Puser, Pall),
	init_hash(A, F, Pall).

link(Name) :-
	need_modules(Ms),
	maplist(module_filename(o), Ms, Mso_),
	sort(Mso_, Mso),
	concat_atom(Ms, ' ', L),
	concat_atom(Mso, ' ', Lo),
	concat_atom(['make PROG="', Name, '" MODULES="', Lo, '" ', Name], Make),
	format(user_error, '~w\n', [Make]),
	format(user_error, '[ Linking module(s) ~w :', [L]),
	shell(Make, R), !,
	(
		R=0
	->
		format(user_error, ' done ]\n', [])
	;
		format(user_error, ' failed ]\n', []),
		flag(error, E, E+1),
		fail
	).

code_module(Mod, I) :+
	get_preds(I, Lpr),
	(
		Mod == user
	->
		get_query(Lpr, Q, P)
	;
		P = Lpr,
		Q = [] 
	),
	get_exports(Us, Xs),
	check_import(Us, Xs),
	export_get(X),
	check_export(X, P, Xs),
	(
		Mod == user
	->
		maplist(export_user_preds, P)
	;
		true
	),
	'$recorded_all'(export_ops, Ops),
	format(mod, '\tops(~q),\n',  [Ops]),
	init_module(Mod, P, Q, X, Xs),
	(
		Mod == user
	->
		code_Q(Q)
	;
		true
	),
	maplist(code_P(Mod),P),
	(
		Mod == system
	->
		code_FPr(Mod)
	;
		true
	),
	code_fin.

init_module(M, P, Q, X, Xs) :-
	'$erase_records'(undef_pred),
	anf_module(M, P, Q, X),
	map_atom(M, Mod),
	used_modules(Ms),
	maplist(decl_import_mod, Ms),
	maplist(decl_export_mod, Xs),
	nl,
	format('\nvoid module_~w(void)\n{\n', [Mod]),
	format('  //if (&&backtrack==0) return;\n\n').

get_atom_from_fun(A/_, A).

init_hash(La, Lf, Lp) :-
	findall(A, atoms(A), Ba),
	findall(F, functors(F), Bf),
	append(La, Ba, Ca),
	append(Lf, Bf, F),
	maplist(get_atom_from_fun, F, Af),
	append(Ca, Af, A),
	sort(A, Sa),
	sort(F, Sf),
	sort(Lp, Sp),
	init_hash_atoms(Sa),
	decl_preds(Sp),
	init_hash_funs(Sf).


code_fin :+
	+> backtrack,
	+> format('}\n').

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
set_meta(F, A) :-
	(
		recorded(meta, F/A)
	->
		flag(meta, _, true)
	;
		flag(meta, _, false)
	).


code_P(M, P) :-
	code_Pr(M, P, T, []),
	trad(T).

code_Pr(M, pr(F, A, [C])) :+
	format('/* code for ~w/~w */\n', [F, A]),
	set_meta(F, A),
	code_C(M, F, A, C, single).
code_Pr(M, pr(F, A, [C|Q])) :+
	format('/* code for ~w/~w */\n', [F, A]),
	set_meta(F, A),
	code_C(M, F, A, C, first),
	code__Pr(M, pr(F, A, Q)).

code__Pr(M, pr(F, A, [C])) :+
	code_C(M, F, A, C, last).
code__Pr(M, pr(F, A, [C|Q])) :+
	code_C(M, F, A, C, middle),
	code__Pr(M, pr(F, A, Q)).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
code_FPr(M) :+
	foreign_preds(full,ndet, Ln),
	foldl(code_FPr_ndet(M), Ln),
	foreign_preds(full, det, Ld),
	foldl(code_FPr_det(M), Ld).

code_FPr_ndet(M, F/N-C) :+
	+> comm_pred(F, N),
	L is N+4,
	L1 is L+1,
	+> flag(arg, _, fp4),
	flag(arg, _, fp4),
	flag(rho, _, L),
	label(F, N, Li),
	flag(curr_C, _, Li),
	label(Li, _),
	+> fl(Li),
	map_pred(F/N, M, Pm),
	map_fun(F/N, Fm),
	(
		exported(F/N)
	->
		+> g0('asm(".globl " ASM_LBL_STR(PRED~w));', [Pm])
	;
		true
	),
	+> g0('asm(ASM_LBL_STR(PRED~w) ":" : : "p" (&&~w_1));', [Pm, Fm]),
	getlabel1(F, N, Lo),
	btinit(first, Lo, N),
	+> pushenv(L1),
	+> g('FP[~w].ctrl=FIRST_CALL;', [L1]),
	+> fl(Lo),
	make_f_args(N, Args),
	+> g('switch (~w(~w&(FP[~w].ctrl)))', [C, Args, L1]),
	+> g('{ case SUCCEED: delbtp();'),
	+> g('                popenv();'),
	+> g('                break;'),
	+> g('  case FAIL:    delbtp();'),
	+> g('                goto backtrack;'),
	+> g('  case RETRY:   FP[~w].ctrl=NEXT_CALL;', [L1]),
	+> g('                restore();'),
	+> g('}\n'), !.

code_FPr_det(M, F/N-C) :+
	+> comm_pred(F, N),
	+> flag(arg, _, arg),
	flag(arg, _, arg),
	flag(rho, _, N),
	label(F, N, Li),
	flag(curr_C, _, Li),
	label(Li, _),
	+> fl(Li),
	map_pred(F/N, M, Pm),
	map_fun(F/N, Fm),
	(
		exported(F/N)
	->
		+> g0('asm(".globl " ASM_LBL_STR(PRED~w));', [Pm])
	;
		true
	),
	+> g0('asm(ASM_LBL_STR(PRED~w) ":" : : "p" (&&~w_1));', [Pm, Fm]),
	getlabel1(F, N, Lo),
	btinit(single, Lo, N),
	+> pushenv(single, 0, N),
	make_ARGs(N, Args),
	+> g('if (!~w(~w))', [C, Args]),
	+> g('  goto backtrack;'),
	fin(single),
	+> nl, !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

code_C(M, F, N, cl(La, G), T) :+
	maplist(trans, La, Lt),
	trans_term(G, Gt),
	vars(Lt, Gt, R),
	label(F, N, Li),
	flag(curr_C, _, Li),
	label(Li, _),
	+> fl(Li),
	+> flag(type_cl, _, T),
	(
		T==single
	->
		flag(rho, _, 0)
	;
		flag(rho, _, N+4)
	),
	(
		(
			T==single
		;
			T==first
		)
	->
		map_pred(F/N, M, Pm),
		map_fun(F/N, Fm),
		(
			exported(F/N)
		->
			+> g0('asm(".globl " ASM_LBL_STR(PRED~w));', [Pm])
		;
			true
		),
		+> g0('asm(ASM_LBL_STR(PRED~w) ":" : : "p" (&&~w_1));', [Pm, Fm]),
		+> flag(arg, _, arg)
	;
		+> flag(arg, _, fp4)
	),
	getlabel1(F, N, Lo),
	btinit(T, Lo, N),
	+> pushenv(T, R, N),
	mapli(code_UA, Lt),
	+> flag(arg, _, fp4),
	code_G(Gt),
	fin(T),
	+> nl,
	'$erase_records'(vars_list).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
reset_fvar(E) :+
	code_X(E, X),
	+> reset_var(X).


code_G((G1, G2)) :+
	code_G(G1),
	code_G(G2).
code_G((C->T)) :+
	code_G((C->T;fail)).
code_G((G1;G2)) :+
	find_fvar(G1, Fv),
	+> alt_0(L_),
	code_G(G1, L),
	code_G_or(G2, Fv, L_, L).
code_G(\+G) :+
	code_G((G->fail;true)).
code_G(not(G)) :+
	code_G(\+G).
code_G(G) :+
	inline(G).
% for inlined builtin code
code_G(G) :+
	code_call(G, L),
	flag(curr_C, C, C),
	label(C, L),
	+> fl(L).
code_G((G1, G2), L) :+
	code_G(G1),
	code_G(G2, L).
code_G((G1->G2), L) :+
	code_G(G1),
	+> cut,
	code_G(G2, L).
code_G((G1;G2), K) :+
	find_fvar(G1, Fv),
	+> alt_0(L_),
	code_G(G1, L),
	code_G_or(G2, Fv, L_, L),
	+> jump(K).
code_G(\+G, K) :+
	code_G((G->fail;true)),
	+> jump(K).
code_G(not(G), K) :+
	code_G(\+G),
	+> jump(K).
code_G(G, K) :+
	inline(G),
	+> jump(K).
code_G(G, L) :+
	code_call(G, L).


code_G_or(fail, F, L_, L) :+
	L_=backtrack,
	+> alt_2,
	foldl(reset_fvar, F),
	flag(curr_C, Li, Li),
	label(Li, L),
	+> fl_(L).
code_G_or((G1;G2), F, L_, L) :+
	flag(curr_C, Li, Li),
	label(Li, L_),
	+> fl(L_),
	+> alt_1(Lb),
	foldl(reset_fvar, F),
	find_fvar(G1, Fv),
	code_G(G1, L),
	code_G_or(G2, Fv, Lb, L).
code_G_or(G, F, L_, L) :+
	flag(curr_C, Li, Li),
	label(Li, L_),
	+> fl(L_),
	+> alt_2,
	foldl(reset_fvar, F),
	code_G(G, L),
	flag(curr_C, Li, Li),
	label(Li, L),
	+> fl_(L).


code_call(G, L) :+
	'$functor'(G, F, N, A),
	check_module(F/N),
	flag(curr_C, C, C),
	getlabel(C, Lab),
	+> comm(Lab),
	flag(meta, Meta, Meta),
	(
		meta_pred(F/N, I),
		Meta\=true
	->
		flag(current_module, Mod, Mod),
		add_module(Mod, I, A, Arg)
	;
		Arg=A
	),
	mapli(code_Arg, Arg),
	+> vm_call(F, N, L).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
code_Q(Q) :-
	trans_term(Q, Qt),
	vars(Qt, R),
	L is R+4,
	flag(rho, _, 4),
	flag(curr_C, _, query),
	label(query, _),
	T=[init, comm('code for query'), pushenv(L)|Tq],
	code_G(Qt, Tq, [vm_halt, failed]),
	trad(T),
	'$erase_records'(vars_list).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
btinit(first, L, N) :+
	+> setbtp(L),
	+> saveargs(N).
btinit(middle, L, _) :+
	+> nextalt(L).
btinit(last, _, _) :+
	+> delbtp.
btinit(single, _, _) :+
	true.

fin(last) :+
	+> popenv.
fin(single) :+
	+> popenv.
fin(first) :+
	+> restore.
fin(middle) :+
	+> restore.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Not used but could be used for an interactive loop
code_binding(Bs) :+
	+> g0('\n\n#ifdef\tINTERACTIVE'),
	+> g0('const char *PL_freevar[] =\n{'),
	foldl(binding, Bs),
	+> g0('};\n'),
	+> g0('int PL_nbr_fv = sizeof(PL_freevar)/sizeof(PL_freevar[0]);'),
	+> g0('#endif\n\n').

binding(N=var(_, I)) :+
	J is I-1,
	+> g0('  [~w] "~w",', [J, N]).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
