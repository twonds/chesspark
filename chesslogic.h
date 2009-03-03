#ifndef __CHESSLOGIC_H__
#define __CHESSLOGIC_H__

enum ChessPiece
{
	CP_EMPTY = 0,
	CP_WPAWN,
	CP_WKNIGHT,
	CP_WBISHOP,
	CP_WROOK,
	CP_WQUEEN,
	CP_WKING,
	CP_BPAWN,
	CP_BKNIGHT,
	CP_BBISHOP,
	CP_BROOK,
	CP_BQUEEN,
	CP_BKING
};

enum ChessTurn
{
	CT_NONE,
	CT_WHITE,
	CT_BLACK
};

enum ChessCastling
{
	CC_NOCC = 0,
	CC_WQ   = 1,
	CC_WK   = 2,
	CC_BQ   = 4,
	CC_BK   = 8
};

int IsPieceWhite(enum ChessPiece piece);
int FindNextPiece(enum ChessPiece board[8][8], enum ChessPiece piece, int *x, int *y);
int IsPathClear(enum ChessPiece board[8][8], int oldx, int oldy, int newx, int newy);
int CanPieceMoveHere(enum ChessPiece board[8][8], int oldx, int oldy, int newx, int newy);
int ExecuteMove(enum ChessPiece board[8][8], char *move, int nopromotions, enum ChessCastling *castling, char **enpassanttarget);
void DumpBoard(enum ChessPiece board[8][8]);
char *InterpretMove(enum ChessPiece board[8][8], char *longmove, char *annotation);
void ChessLogic_LoadDefaultBoard(enum ChessPiece board[8][8]);
void ChessLogic_CalcScore(enum ChessPiece board[8][8], int *whitescore, int *blackscore);
void ChessLogic_ParseFEN(char *fen, enum ChessPiece board[8][8], enum ChessTurn *turn, enum ChessCastling *castling, char **enpassanttarget);

#endif