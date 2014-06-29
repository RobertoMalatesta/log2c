/************************************************************************/
/* Copyright (c) 1998-2014 Luc Van Oostenryck. All rights reserved.	*/
/*									*/
/* %@%LICENSE								*/
/*									*/
/************************************************************************/

:- module(aux, [
		a_n_f/6,
		del/1,
		del_all/0,
		export_pred/1,
		exported/1,
		file_type/2,
		flag2/3,
		module_filename/3,
		noescape/2,
		read_all/2,
		read_export/2,
		read_module/1,
		to_list/2
	]).

:- use_module(errmsg).
:- use_module(foreign).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%!	file_type(+File, -Type) is semidet
%
%	Open File, return the corresponding Stream
%	and return the 'type' of the file:
%	- 'module(M, X)' is the file begins with a module directive
%	- 'user' otherwise
file_type(F, T) :-
	file_base_name(F, Name),
	file_name_extension(Base, Ext, Name),
	(   
		Ext==pl
	->
		true
	;
		warning('~w : may not be a Prolog file', [F])
	),
	open(F, read, S, [alias(src)]),
	stream_property(S, position(P)),
	read(S, R),
	(   
		R= (:-module(M, L))
	->
		(   
			module_filename(pl, M, Name)
		->
			true
		;
			warning('file (~w) and module (~w) do not match', [F, M])
		),
		T=module(M, L)
	;
		set_stream_position(S, P),
		T=user
	),
	flag(input_file, _, Base).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

read_module(L) :-
	readclauses_(src, C, []),
	flag(current_module, M, M),
	(   
		M==system
	->
		L=C
	;
		
		concat($, _, M)
	->
		L=C
	;
		L=[ (:-use_module(system))|C]
	).

readclauses([], O, O) :- !.
readclauses([F|Q], I, O) :-
	readclauses(F, I, T),
	readclauses(Q, T, O), !.
readclauses(F, I, O) :-
	open(F, read, S, []),
	readclauses_(S, I, O),
	close(S).

readclauses_(S, I, O) :-
	read_term(S, T, [variable_names(V)]),
	(   
		T==end_of_file
	->
		I=O
	;
		expand_term(T, Tx),
		read_Pr(Tx, V, I, Tmp),
		readclauses_(S, Tmp, O)
	), !.

read_Pr((main:-Q), V, I, O) :-
	read_Pr((:-main(Q, V)), V, I, O).
read_Pr((:-consult(F)), _, I, O) :-
	readclauses(F, I, O).
read_Pr((:-include(F)), _, I, O) :-
	readclauses(F, I, O).
read_Pr((:-op(P, T, N)), _, [ (:-op(P, T, N))|O], O) :-
	op(P, T, N).

read_Pr(T, _, [T|O], O).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
del_all :-
	'$erase_records'(code),
	'$erase_records'(vars_list),
	'$erase_records'(directive),
	'$erase_records'(curr_C),
	'$erase_records'(indent),
	'$erase_records'(preds),
	'$erase_records'(export_pred),
	'$erase_records'(used_modules),
	'$erase_records'(module_compiled),
	flag(indent, _, 0).

del(K) :-
	'$erase_records'(K).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
noescape(F, Fs) :-
	atom_codes(F, L),
	noescape_(L, Ls),
	atom_codes(Fs, Ls).
noescape_([], []).
noescape_([92|Q], [92, 92|R]) :-
	noescape_(Q, R).
noescape_([34|Q], [92, 34|R]) :-
	noescape_(Q, R).
noescape_([10|Q], [92, 110|R]) :-
	noescape_(Q, R).
noescape_([E|Q], [E|R]) :-
	noescape_(Q, R).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
to_list(T, R) :-
	to_list(T, R, []).

to_list(A, I, O) :-
	var(A), !,
	I=[A|O].
to_list((A, B), I, O) :-
	to_list(A, I, T),
	to_list(B, T, O).
to_list((A;B), I, O) :-
	to_list(A, I, T),
	to_list(B, T, O).
to_list((A| B), I, O) :-
	to_list(A, I, T),
	to_list(B, T, O).
to_list((A->B), I, O) :-
	to_list(A, I, T),
	to_list(B, T, O).
to_list((A*->B), I, O) :-
	to_list(A, I, T),
	to_list(B, T, O).
to_list(\+A, I, O) :-
	to_list(A, I, O).
to_list(not(A), I, O) :-
	to_list(A, I, O).
to_list(E, [E|O], O).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
a_n_f(G, Q, X, A, F, P) :-
	flag(current_module, M, M),
	(   
		M=0
	->
		true
	;
		anf_rec_atom(M)
	),
	anf_clause(cl([], Q)),
	maplist(anf_pred, G),
	maplist(anf_rec_fun, X),
	anf(A, F, P).

anf(A, F, P) :-
	flag(current_module, M, M),
	(   
		M==system
	->
		foreign_preds(Fps),
		maplist(aux:anf_rec_pred, Fps)
	;
		true
	),
	recorded(used_modules, Ms),
	maplist(aux:anf_rec_atom, Ms),
	anf_rec_import,
	anf_get_atom(A),
	anf_get_fun(F),
	anf_get_pred(P).


anf_rec_import :-
	recorded(module_export, module_export(_, F/_)),
	anf_rec_atom(F),
	fail.
anf_rec_import.


anf_pred(pr(F, N, L)) :-
	anf_rec_pred(F/N),
	maplist(anf_clause, L).

anf_clause(cl(H, G)) :-
	maplist(anf_elem,H),
	to_list(G, L),
	maplist(anf_goal, L).

anf_goal(E) :-
	compound(E), !,
	%% We do not record the goal's atoms & functors, only the ones in their arguments
	E=..[_|L],
	maplist(anf_elem, L).
anf_goal(_).

anf_elem(E) :-
	compound(E), !,
	functor(E, F, N),
	anf_rec_fun(F, N),
	E=..[F|L],
	maplist(anf_elem, L).
anf_elem(E) :-
	atom(E), !,
	anf_rec_atom(E).
anf_elem(_).


anf_rec_atom(A) :-
	recorda(anf_rec_atom, A).

anf_rec_fun(F/N) :-
	anf_rec_fun(F, N).
anf_rec_fun(F, N) :-
	recorda(anf_rec_fun, F/N),
	anf_rec_atom(F).	%% FIXME: Needed? can be done at collect time

anf_rec_pred(P) :-
	recorda(anf_rec_pred, P),
	anf_rec_fun(P),		%% FIXME: Why de we really need that?
	true.


anf_get_erase(K, S) :-
	'$recorded_all'(K, L),
	sort(L, S),
	'$erase_records'(K).

anf_get_atom(A) :-
	anf_get_erase(anf_rec_atom, A).
anf_get_fun(F) :-
	anf_get_erase(anf_rec_fun, F).
anf_get_pred(P) :-
	anf_get_erase(anf_rec_pred, P).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Read all terms of file 'F' and put it in list 'L'

read_all(F, L) :-
	current_input(Old),
	open(F, read, N),
	set_input(N),
	read_all_(L),
	close(N),
	set_input(Old).

read_all_(L) :-
	read(T),
	(   
		T==end_of_file
	->
		L=[]
	;
		read_all_(Q),
		L=[Tx|Q],
		expand_term(T, Tx)
	).

read_export(F, X) :-
	current_input(Old),
	open(F, read, N),
	set_input(N),
	read_x_(X),
	close(N),
	set_input(Old).

read_x_(X) :-
	read(T),
	(   
		T==end_of_file
	->
		fail
	;
		
		T=export(X)
	->
		true
	;
		read_x_(X)
	).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
flag2(K, V1, V2) :-
	(   
		var(V1),
		var(V2)
	->
		W1=V1,
		W2=V2
	;
		true
	),
	concat(K, '_1', K1),
	flag(K1, W1, V1),
	concat(K, '_2', K2),
	flag(K2, W2, V2).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
export_pred(Xs) :-
	format(mod, 'export(~q).\n', [Xs]),
	map_recorda(export_pred, Xs).
map_recorda(_, []).
map_recorda(K, [A|Q]) :-
	recorda(K, A),
	map_recorda(K, Q).

exported(P) :-
	recorded(export_pred, P).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%! module_base(+Module, -Basename) is det
%
% Return the basename corresponding to a module.
module_basename(user, B) :-
	!,
	flag(input_file, B, B).
module_basename(M, B) :-
	(
		concat($, R, M)
	->
		B = R
	;
		B = M
	).

%! module_filename(+Extension, +Module, -Basename) is det
%
% Return the filename corresponding to a module.
module_filename(X, M, F) :-
	module_basename(M, B),
	file_name_extension(B, X, F).
