%% Copyright (c) 1997 Luc Van Oostenryck. All rights reserved.
%%

:- module(trans,[ trans_term/2
		, trans/2
		]).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

fun(T,F,N,A)	:- compound(T), functor(T,F,N), T=..[F|A].

trans_term((A,B),(X,Y))		:- trans_term(A,X), trans_term(B,Y).
trans_term((A;B),(X;Y))		:- trans_term(A,X), trans_term(B,Y).
trans_term((A|B),(X|Y))		:- trans_term(A,X), trans_term(B,Y).
trans_term((A->B),(X->Y))	:- trans_term(A,X), trans_term(B,Y).
trans_term((A*->B),(X*->Y))	:- trans_term(A,X), trans_term(B,Y).
trans_term(not(A),not(X))	:- trans_term(A,X).
trans_term(\+(A),\+(X))		:- trans_term(A,X).
trans_term(A,X)		:- A=..[F|La],
			   maplist(trans,La,Lx),
			   X=..[F|Lx].

trans(A,X)		:- atom(A), X=atom(A).
trans(A,X)		:- integer(A), X=intg(A).
trans(A,A)		:- var(A).
%% trans(A,X)		:- float(A), X=float(A).
%% trans(A,X)		:- string(A), X=string(A).
trans(T,X)		:- fun(T,F,N,A),
			   maplist(trans,A,At),
			   X=fun(F,N,At).
