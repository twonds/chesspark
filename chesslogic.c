#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#include "util.h"

#include "chesslogic.h"

void CopyBoard(enum ChessPiece srcboard[8][8], enum ChessPiece dstboard[8][8])
{
	int x, y;
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			dstboard[x][y] = srcboard[x][y];
		}
	}
}

enum ChessPiece ConvertCharToPiece(char cpiece)
{
	char *pieces = "oPNBRQKpnbrqk";

	return (int)(strchr(pieces, cpiece) - pieces);
}

enum ChessPiece SwapPieceColor(enum ChessPiece type)
{
	if (type > CP_WKING)
	{
		return type - 6;
	}
	else if (type != CP_NONE)
	{
		return type + 6;
	}

	return type;
}

int IsPieceWhite(enum ChessPiece piece)
{
	if (piece >= CP_WPAWN && piece <= CP_WKING)
	{
		return 1;
	}
	return 0;
}

int FindNextPiece(enum ChessPiece board[8][8], enum ChessPiece piece, int *x, int *y)
{
	if (*x == -1)
	{
		*x = -1;
		*y = 0;
	}

	while (1)
	{
		(*x)++;
		if (*x == 8)
		{
			*x = 0;
			(*y)++;
			if (*y == 8)
				return 0;
		}

		if (board[*x][*y] == piece)
		{
			return 1;
		}
	}
}

int IsPathClear(enum ChessPiece board[8][8], int oldx, int oldy, int newx, int newy)
{
	int dx = (newx == oldx) ? 0 : ((newx > oldx) ? 1 : -1);
	int dy = (newy == oldy) ? 0 : ((newy > oldy) ? 1 : -1);
	int x = oldx;
	int y = oldy;

	/* straight lines and diagonals only */
	if ((abs(oldx - newx) != abs(oldy - newy)) && (oldx != newx) && (oldy != newy))
	{
		return 0;
	}

	while ((x != newx) || (y != newy))
	{
		x += dx;
		y += dy;

		if (board[x][y] != CP_EMPTY && !((x == newx) && (y == newy) && (IsPieceWhite(board[newx][newy]) != IsPieceWhite(board[oldx][oldy]))))
		{
			return 0;
		}
	}

	return 1;
}

int CanPieceMoveHere(enum ChessPiece board[8][8], int oldx, int oldy, int newx, int newy, enum ChessCastling *castling, char **enpassanttarget)
{
	enum ChessPiece moving = board[oldx][oldy];
	enum ChessPiece captured = board[newx][newy];
	char *letters = "abcdefgh";
	char *numbers = "87654321";
	int epx, epy;

	if (oldx == newx && oldy == newy)
	{
		return 0;
	}
	
	if (enpassanttarget && *enpassanttarget)
	{
		epx = strchr(letters, (*enpassanttarget)[0]) - letters;
		epy = strchr(numbers, (*enpassanttarget)[1]) - numbers;
	}
	else
	{
		epx = -1;
		epy = -1;
	}

	switch(moving)
	{
		case CP_WPAWN:
		{
			if (!IsPathClear(board, oldx, oldy, newx, newy))
			{
				return 0;
			}
			if (oldx == newx)
			{
				if (!captured && (newy == oldy - 1 || (newy == 4 && oldy == 6)))
				{
					return 1;
				}
			}
			else if ((abs(oldx - newx) == 1) && (newy == oldy - 1))
			{
				if (captured != CP_EMPTY && !IsPieceWhite(captured))
				{
					return 1;
				}
				else if (captured == CP_EMPTY && newx == epx && newy == epy)
				{
					return 1;
				}
			}
			return 0;
		}
		break;

		case CP_BPAWN:
		{
			if (!IsPathClear(board, oldx, oldy, newx, newy))
			{
				return 0;
			}
			if (oldx == newx)
			{
				if (!captured && (newy == oldy + 1 || (newy == 3 && oldy == 1)))
				{
					return 1;
				}
			}
			else if ((abs(oldx - newx) == 1) && (newy == oldy + 1))
			{
				if (captured != CP_EMPTY && IsPieceWhite(captured))
				{
					return 1;
				}
				else if (captured == CP_EMPTY && newx == epx && newy == epy)
				{
					return 1;
				}
			}
			return 0;
		}

		case CP_WKNIGHT:
		case CP_BKNIGHT:
		{
			if (!((abs(oldx - newx) == 2 && abs(oldy - newy) == 1) || (abs(oldx - newx) == 1 && abs(oldy - newy) == 2)))
			{
				return 0;
			}
			if (captured && IsPieceWhite(captured) == IsPieceWhite(moving))
			{
				return 0;
			}
			return 1;
		}
		break;

		case CP_WBISHOP:
		case CP_BBISHOP:
		{
			if (!IsPathClear(board, oldx, oldy, newx, newy))
			{
				return 0;
			}
			if (abs(oldx - newx) != abs(oldy - newy))
			{
				return 0;
			}
			return 1;
		}
		break;

		case CP_WROOK:
		case CP_BROOK:
		{
			if (!IsPathClear(board, oldx, oldy, newx, newy))
			{
				return 0;
			}
			if ((oldx - newx != 0) && (oldy - newy != 0))
			{
				return 0;
			}
			return 1;
		}

		case CP_WQUEEN:
		case CP_BQUEEN:
		{
			if (!IsPathClear(board, oldx, oldy, newx, newy))
			{
				return 0;
			}
			if (((oldx - newx != 0) && (oldy - newy != 0)) && (abs(oldx - newx) != abs(oldy - newy)))
			{
				return 0;
			}
			return 1;
		}

		case CP_WKING:
		case CP_BKING:
		{
			if (!IsPathClear(board, oldx, oldy, newx, newy))
			{
				return 0;
			}
			if (abs(oldx - newx) > 1 || abs(oldy - newy) > 1)
			{
				/* check for castling */
				if (castling)
				{

					if (moving == CP_WKING && oldx == 4 && oldy == 7 && newy == 7)
					{
						if (newx == 6 && (*castling & CC_WK) != 0)
						{
							return 1;
						}
						if (newx == 2 && (*castling & CC_WQ) != 0)
						{
							return 1;
						}
					}

					if (moving == CP_BKING && oldx == 4 && oldy == 0 && newy == 0)
					{
						if (newx == 6 && (*castling & CC_BK) != 0)
						{
							return 1;
						}
						if (newx == 2 && (*castling & CC_BQ) != 0)
						{
							return 1;
						}
					}
				}
				return 0;
			}
			return 1;
		}
		break;

		default:
			break;
	}

	return 0;
}

int IsMoveLegal(enum ChessPiece board[8][8], int oldx, int oldy, int newx, int newy, enum ChessCastling *castling, char **enpassanttarget)
{
	enum ChessPiece tempboard[8][8];
	char *letters = "abcdefgh";
	char *numbers = "87654321";
	char move[5];
	int iswhitemove = IsPieceWhite(board[oldx][oldy]);

	if (CanPieceMoveHere(board, oldx, oldy, newx, newy, castling, enpassanttarget))
	{
		/* is this a castling move?  Then check the intermediate square */
		if (oldx == 4 && oldy == 7 && newy == 7 && (newx == 6 || newx == 2) && board[oldx][oldy] == CP_WKING)
		{
			CopyBoard(board, tempboard);

			if (newx == 6)
			{
				strcpy(move, "e8f8");
				ExecuteMove(tempboard, move, 0, NULL, NULL);
				if (!IsPositionLegal(tempboard, !iswhitemove))
				{
					return 0;
				}
			}

			if (newx == 2)
			{
				strcpy(move, "e8d8");
				ExecuteMove(tempboard, move, 0, NULL, NULL);
				if (!IsPositionLegal(tempboard, !iswhitemove))
				{
					return 0;
				}
			}
		}

		if (oldx == 4 && oldy == 7 && newy == 0 && (newx == 6 || newx == 2) && board[oldx][oldy] == CP_BKING)
		{
			CopyBoard(board, tempboard);

			if (newx == 6)
			{
				strcpy(move, "e1f1");
				ExecuteMove(tempboard, move, 0, NULL, NULL);
				if (!IsPositionLegal(tempboard, !iswhitemove))
				{
					return 0;
				}
			}

			if (newx == 2)
			{
				strcpy(move, "e1d1");
				ExecuteMove(tempboard, move, 0, NULL, NULL);
				if (!IsPositionLegal(tempboard, !iswhitemove))
				{
					return 0;
				}
			}
		}

		CopyBoard(board, tempboard);
		move[0] = letters[oldx];
		move[1] = numbers[oldy];
		move[2] = letters[newx];
		move[3] = numbers[newy];
		move[4] = '\0';

		ExecuteMove(tempboard, move, 0, NULL, NULL);
		return IsPositionLegal(tempboard, !iswhitemove);
	}

	return 0;
}


int ExecuteMove(enum ChessPiece board[8][8], char *move, int nopromotions, enum ChessCastling *castling, char **enpassanttarget)
{
	int capture = 0;
	enum ChessPiece newboard[8][8];
	int i, j;

	char *letters = "abcdefgh";
	char *numbers = "87654321";
	char *pieces = "oPNBRQKpnbrqk";

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			newboard[i][j] = board[i][j];
		}
	}

	if (enpassanttarget)
	{
		free(*enpassanttarget);
		*enpassanttarget = NULL;
	}

	while (move && *move)
	{
		char *space;
		int oldx, oldy, newx, newy;
		enum ChessPiece piece, newpiece;
		int length;
			
		length = (int)strlen(move);
		space  = strchr(move, ' ');

		if (space && length > space - move)
		{
			length = (int)(space - move);
		}

		oldx = (int)(strchr(letters, move[0]) - letters);
		oldy = (int)(strchr(numbers, move[1]) - numbers);
		piece = board[oldx][oldy];
		newpiece = piece;

		switch(length)
		{
			case 3: /* Placing a piece on the board, or erasing a square */
				newboard[oldx][oldy] = (int)(strchr(pieces, move[2]) - pieces);
				capture = 2;
				break;
			
			case 5: /* Promotion */
			{
				char *p;
				p = strchr(pieces, move[4]);

				if (p)
				{
					if (!nopromotions)
					{
						newpiece = (int)(p - pieces);

						if (piece <= CP_WKING && newpiece > CP_WKING)
						{
							newpiece = newpiece - 6 ;
						}
						else if (piece > CP_WKING && newpiece <= CP_WKING)
						{
							newpiece = newpiece + 6;
						}
					}
				}
				else
				{
					Log_Write(0, "Error parsing promotion!\n");
				}
			}

			case 4: /* Normal move */
			case 8:
				newx = (int)(strchr(letters, move[2]) - letters);
				newy = (int)(strchr(numbers, move[3]) - numbers);

				if (board[newx][newy] != CP_EMPTY)
				{
					capture = 1;
				}

				if (newboard[oldx][oldy] == piece)
				{
					newboard[oldx][oldy] = CP_EMPTY;
				}

				/* eliminate castling possibilities */
				if (castling)
				{
					if (piece == CP_WKING)
					{
						*castling &= ~CC_WQ;
						*castling &= ~CC_WK;
					}

					if (piece == CP_BKING)
					{
						*castling &= ~CC_BQ;
						*castling &= ~CC_BK;
					}

					if (piece == CP_WROOK)
					{
						if (oldx == 0 && oldy == 7)
						{
							*castling &= ~CC_WQ;
						}
						else if (oldx == 7 && oldy == 7)
						{
							*castling &= ~CC_WK;
						}
					}

					if (piece == CP_BROOK)
					{
						if (oldx == 0 && oldy == 0)
						{
							*castling &= ~CC_BQ;
						}
						else if (oldx == 7 && oldy == 0)
						{
							*castling &= ~CC_BK;
						}
					}

					if (board[newx][newy] == CP_WROOK && newx == 0 && newy == 7)
					{
						*castling &= ~CC_WQ;
					}

					if (board[newx][newy] == CP_WROOK && newx == 7 && newy == 7)
					{
						*castling &= ~CC_WK;
					}

					if (board[newx][newy] == CP_WROOK && newx == 0 && newy == 0)
					{
						*castling &= ~CC_BQ;
					}

					if (board[newx][newy] == CP_WROOK && newx == 7 && newy == 0)
					{
						*castling &= ~CC_BK;
					}
				}

				/* set en passant capture point */
				if (enpassanttarget)
				{
					char txt[256];

					if (piece == CP_WPAWN)
					{
						if (oldy == 6 && newy == 4)
						{
							sprintf(txt, "%c%c", letters[oldx], numbers[5]);
							*enpassanttarget = strdup(txt);
						}
					}
					else if (piece == CP_BPAWN)
					{
						if (oldy == 1 && newy == 3)
						{
							sprintf(txt, "%c%c", letters[oldx], numbers[2]);
							*enpassanttarget = strdup(txt);
						}
					}
				}

				newboard[newx][newy] = newpiece;
				break;
			default:
				break;
		}

		while (space && *space == ' ')
		{
			space++;
		}

		move = space;
	}

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			board[i][j] = newboard[i][j];
		}
	}

	return capture;
}

void DumpBoard(enum ChessPiece board[8][8])
{
	char txt[10];
	char *pieces = ".PNBRQKpnbrqk";
	int x, y;

	for(y=0;y<8;y++)
	{
		for(x=0;x<8;x++)
		{
			txt[x] = pieces[board[x][y]];
		}
		txt[8] = '\n';
		txt[9] = '\0';
		Log_Write(0, txt);
	}

}

char *InterpretMove(enum ChessPiece board[8][8], char *longmove, char *annotation)
{
	char *shortmove = malloc(20);
	int showrank = 0;
	int showfile = 0;
	enum ChessPiece moving, captured;
	char *letters = "abcdefgh";
	char *numbers = "87654321";
	char *pieces = "oPNBRQKPNBRQK";
	char *space = strchr(longmove, ' ');
	int x, y, oldx, oldy, newx, newy;

	/*Log_Write(0, "InterpretMove(board, %s, %s)\n", longmove, annotation);*/

	if (strlen(longmove) == 3)
	{
		/* Piece placement */
		strcpy (shortmove, longmove);

		return shortmove;
	}

	oldx = (int)(strchr(letters, longmove[0]) - letters);
	oldy = (int)(strchr(numbers, longmove[1]) - numbers);

	moving = board[oldx][oldy];
	
	if ((space && space - longmove > 3) || (!space && strlen(longmove) > 3))
	{
		newx = (int)(strchr(letters, longmove[2]) - letters);
		newy = (int)(strchr(numbers, longmove[3]) - numbers);

		captured = board[newx][newy];
	}

	x = -1;

	/*Log_Write(0, "looking for a conflict to %s, piece %d\n", longmove, moving);*/

	/*DumpBoard(board);*/

	if ((space && space - longmove > 3) || (!space && strlen(longmove) > 3))
	{

	while(FindNextPiece(board, moving, &x, &y))
	{
		/*Log_Write(0, "found a piece %d at (%d, %d) -> Can piece move to (%d, %d)? %d\n", moving, x, y, newx, newy, CanPieceMoveHere(board, x, y, newx, newy));*/
		if (!(x == oldx && y == oldy) && CanPieceMoveHere(board, x, y, newx, newy, NULL, NULL))
		{
			if (x == oldx)
			{
				showrank = 1;
			}
			else
			{
				showfile = 1;
			}
		}
	}

	}

	shortmove[0] = '\0';

	if (strlen(longmove) == 9 && (moving == CP_WKING || moving == CP_BKING))
	{
		if (newx > oldx)
		{
			strcpy(shortmove, "O-O");
		}
		else
		{
			strcpy(shortmove, "O-O-O");
		}
	}
	else if (strlen(longmove) >= 4 && strlen(longmove) <= 5)
	{
		if (!(moving == CP_WPAWN || moving == CP_BPAWN))
		{
			strncat(shortmove, &(pieces[moving]), 1);
		}
		if (showfile || ((moving == CP_WPAWN || moving == CP_BPAWN) && captured))
		{
			strncat(shortmove, &(longmove[0]), 1);
		}
		if (showrank)
		{
			strncat(shortmove, &(longmove[1]), 1);
		}
		if (captured)
		{
			strcat(shortmove, "x");
		}
		strncat(shortmove, &(longmove[2]), 2);
		if (strlen(longmove) == 5)
		{
			char *upcase;
			strcat(shortmove, "=");

			upcase = Util_Capitalize(&(longmove[4]));
			strncat(shortmove, upcase, 1);
			free(upcase);
		}
	}

	else if (strlen(longmove) == 8) /* en passant */
	{
		strncat(shortmove, &(longmove[0]), 1);
		strcat(shortmove, "x");
		strncat(shortmove, &(longmove[2]), 2);
	}
	else
	{
		strcpy (shortmove, "???");
	}

	if (annotation && stricmp(annotation, "check") == 0)
	{
		strcat(shortmove, "+");
	}
	else if (annotation && stricmp(annotation, "checkmate") == 0)
	{
		strcat(shortmove, "#");
	}

	return shortmove;
}

char *GetCaptureText(enum ChessPiece board[8][8], char *longmove)
{
	char *letters = "abcdefgh";
	char *numbers = "87654321";
	char *pieces = "oPNBRQKpnbrqk";
	char *submove;
	char txt[512];
	enum ChessPiece tempboard[8][8];

	txt[0] = '\0';
	submove = strdup(longmove);

	/* filter for castling */
	if (strlen(longmove) == 9 && longmove[4] == ' ')
	{
		int x1, y1, x2, y2;

		x1 = (int)(strchr(letters, longmove[0]) - letters);
		y1 = (int)(strchr(numbers, longmove[1]) - numbers);

		x2 = (int)(strchr(letters, longmove[5]) - letters);
		y2 = (int)(strchr(numbers, longmove[6]) - numbers);

		if ((board[x1][y1] == CP_WKING && board[x2][y2] == CP_WROOK)
		 || (board[x1][y1] == CP_WROOK && board[x2][y2] == CP_WKING) 
		 || (board[x1][y1] == CP_BKING && board[x2][y2] == CP_BROOK) 
		 || (board[x1][y1] == CP_WROOK && board[x2][y2] == CP_WKING))
		{
			return "";
		}
	}

	CopyBoard(board, tempboard);

	do {
		char *space = strchr(submove, ' ');
		int x, y, len;
		enum ChessPiece captured;
		
		if (space)
		{
			len = space - submove;
			*space = '\0';
		}
		else
		{
			len = strlen(submove);
		}

		if (len == 3)
		{
			x = (int)(strchr(letters, submove[0]) - letters);
			y = (int)(strchr(numbers, submove[1]) - numbers);
		}
		else
		{
			x = (int)(strchr(letters, submove[2]) - letters);
			y = (int)(strchr(numbers, submove[3]) - numbers);
		}

		captured = tempboard[x][y];

		if (tempboard[x][y] != CP_NONE)
		{
			strncat(txt, &(pieces[captured]), 1);
			strncat(txt, &(letters[x]), 1);
			strncat(txt, &(numbers[y]), 1);
		}

		ExecuteMove(tempboard, submove, 0, NULL, NULL);

		if (space)
		{
			submove = space + 1;
		}
		else
		{
			submove = NULL;
		}

	} while (submove && *submove);

	return strdup(txt);
}

char IsPiecePlacement(char *move)
{
	if (strlen(move) == 3 && move[2] != 'o')
	{
		return move[2];
	}
	return NULL;
}

void ChessLogic_LoadDefaultBoard(enum ChessPiece board[8][8])
{
	board[0][0] = CP_BROOK;
	board[1][0] = CP_BKNIGHT;
	board[2][0] = CP_BBISHOP;
	board[3][0] = CP_BQUEEN;
	board[4][0] = CP_BKING;
	board[5][0] = CP_BBISHOP;
	board[6][0] = CP_BKNIGHT;
	board[7][0] = CP_BROOK;

	board[0][1] = CP_BPAWN;
	board[1][1] = CP_BPAWN;
	board[2][1] = CP_BPAWN;
	board[3][1] = CP_BPAWN;
	board[4][1] = CP_BPAWN;
	board[5][1] = CP_BPAWN;
	board[6][1] = CP_BPAWN;
	board[7][1] = CP_BPAWN;

	board[0][2] = CP_EMPTY;
	board[1][2] = CP_EMPTY;
	board[2][2] = CP_EMPTY;
	board[3][2] = CP_EMPTY;
	board[4][2] = CP_EMPTY;
	board[5][2] = CP_EMPTY;
	board[6][2] = CP_EMPTY;
	board[7][2] = CP_EMPTY;

	board[0][3] = CP_EMPTY;
	board[1][3] = CP_EMPTY;
	board[2][3] = CP_EMPTY;
	board[3][3] = CP_EMPTY;
	board[4][3] = CP_EMPTY;
	board[5][3] = CP_EMPTY;
	board[6][3] = CP_EMPTY;
	board[7][3] = CP_EMPTY;

	board[0][4] = CP_EMPTY;
	board[1][4] = CP_EMPTY;
	board[2][4] = CP_EMPTY;
	board[3][4] = CP_EMPTY;
	board[4][4] = CP_EMPTY;
	board[5][4] = CP_EMPTY;
	board[6][4] = CP_EMPTY;
	board[7][4] = CP_EMPTY;

	board[0][5] = CP_EMPTY;
	board[1][5] = CP_EMPTY;
	board[2][5] = CP_EMPTY;
	board[3][5] = CP_EMPTY;
	board[4][5] = CP_EMPTY;
	board[5][5] = CP_EMPTY;
	board[6][5] = CP_EMPTY;
	board[7][5] = CP_EMPTY;

	board[0][6] = CP_WPAWN;
	board[1][6] = CP_WPAWN;
	board[2][6] = CP_WPAWN;
	board[3][6] = CP_WPAWN;
	board[4][6] = CP_WPAWN;
	board[5][6] = CP_WPAWN;
	board[6][6] = CP_WPAWN;
	board[7][6] = CP_WPAWN;

	board[0][7] = CP_WROOK;
	board[1][7] = CP_WKNIGHT;
	board[2][7] = CP_WBISHOP;
	board[3][7] = CP_WQUEEN;
	board[4][7] = CP_WKING;
	board[5][7] = CP_WBISHOP;
	board[6][7] = CP_WKNIGHT;
	board[7][7] = CP_WROOK;
}

void ChessLogic_CalcScore(enum ChessPiece board[8][8], int *whitescore, int *blackscore)
{
	int x, y;

	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			switch(board[x][y])
			{
				case CP_WPAWN:
					*whitescore += 1;
					break;
				case CP_WKNIGHT:
				case CP_WBISHOP:
					*whitescore += 3;
					break;
				case CP_WROOK:
					*whitescore += 5;
					break;
				case CP_WQUEEN:
					*whitescore += 9;
					break;
				case CP_WKING:
					break;
				case CP_BPAWN:
					*blackscore += 1;
					break;
				case CP_BKNIGHT:
				case CP_BBISHOP:
					*blackscore += 3;
					break;
				case CP_BROOK:
					*blackscore += 5;
					break;
				case CP_BQUEEN:
					*blackscore += 9;
					break;
				case CP_BKING:
					break;
				default:
					break;
			}
		}
	}
}

void ChessLogic_ParseFEN(char *fen, enum ChessPiece board[8][8], enum ChessTurn *turn, enum ChessCastling *castling, char **enpassanttarget)
{
	char *numbers = "012345678";
	char *pieces = " PNBRQKpnbrqk";

	char *files = "abcdefgh";
	char *ranks = "87654321";

	char *p = fen;
	int x, y;

	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			board[x][y] = CP_EMPTY;
		}
	}

	x = 0;
	y = 0;

	while (*p && *p != ' ')
	{
		int pos;

		if (*p == '/')
		{
			x = 0;
			y++;
		}
		else if ((pos = (int)(strchr(numbers, *p))))
		{
			pos -= (int)numbers;
			x += pos;
		}
		else if ((pos = (int)(strchr(pieces, *p))))
		{
			pos -= (int)pieces;
			board[x][y] = pos;
			x++;
		}

		p++;
	}

	if (turn)
	{
		*turn = CT_NONE;
	}


	if (*p == ' ')
	{
		p++;
	}

	if (*p == 'w')
	{
		if (turn)
		{
			*turn = CT_WHITE;
		}
		p++;
	}
	else
	{
		if (turn)
		{
			*turn = CT_BLACK;
		}
		p++;
	}

	if (castling)
	{
		*castling = CC_NOCC;
	}

	if (*p == ' ')
	{
		p++;
	}

	while (*p && *p != ' ')
	{
		if (castling)
		{
			if (*p == 'K')
			{
				*castling |= CC_WK;
			}
			else if (*p == 'Q')
			{
				*castling |= CC_WQ;
			}
			else if (*p == 'k')
			{
				*castling |= CC_BK;
			}
			else if (*p == 'q')
			{
				*castling |= CC_BQ;
			}
		}

		p++;
	}

	if (*p == ' ')
	{
		p++;
	}

	if (enpassanttarget)
	{
		free(*enpassanttarget);
		*enpassanttarget = NULL;
	}

	if (*p != '\0' && *p != '-')
	{
		if (enpassanttarget)
		{
			char *ept = malloc(3);
			
			ept[0] = *p++;
			ept[1] = *p++;
			ept[2] = '\0';

			*enpassanttarget = ept;
		}
	}
}

int IsPositionLegal(enum ChessPiece board[8][8], int whitesmove)
{
	int x, y, kingx = -1, kingy = -1;
	if (whitesmove)
	{
		for (x = 0; x < 8; x++)
		{
			for (y = 0; y < 8; y++)
			{
				if (board[x][y] == CP_BKING)
				{
					if (kingx != -1)
					{
						/* no multiple kings */
						return 0;
					}
					kingx = x;
					kingy = y;
				}
			}
		}

		if (kingx == -1)
		{
			/* missing king */
			return 0;
		}

		/* search the board for white pieces, and if they can cap the king */
		for (x = 0; x < 8; x++)
		{
			for (y = 0; y < 8; y++)
			{
				enum ChessPiece piece = board[x][y];
				char *letters = "abcdefgh";
				char *numbers = "87654321";

				if (piece >= CP_WPAWN && piece <= CP_WKING)
				{
					Log_Write(0, "white %d on %c%c can cap king at %c%c? %d.\n", piece, letters[x], numbers[y], letters[kingx], numbers[kingy], CanPieceMoveHere(board, x, y, kingx, kingy, NULL, NULL));
				}

				if (piece >= CP_WPAWN && piece <= CP_WKING && CanPieceMoveHere(board, x, y, kingx, kingy, NULL, NULL))
				{
					return 0;
				}
			}
		}

		return 1;
	}
	else
	{
		for (x = 0; x < 8; x++)
		{
			for (y = 0; y < 8; y++)
			{
				if (board[x][y] == CP_WKING)
				{
					if (kingx != -1)
					{
						/* no multiple kings */
						return 0;
					}
					kingx = x;
					kingy = y;
				}
			}
		}

		if (kingx == -1)
		{
			/* missing king */
			return 0;
		}

		/* search the board for black pieces, and if they can cap the king */
		for (x = 0; x < 8; x++)
		{
			for (y = 0; y < 8; y++)
			{
				enum ChessPiece piece = board[x][y];
				char *letters = "abcdefgh";
				char *numbers = "87654321";

				if (piece >= CP_WPAWN && piece <= CP_WKING)
				{
					Log_Write(0, "black %d on %c%c can cap king at %c%c? %d.\n", piece, letters[x], numbers[y], letters[kingx], numbers[kingy], CanPieceMoveHere(board, x, y, kingx, kingy, NULL, NULL));
				}

				if (piece >= CP_BPAWN && piece <= CP_BKING && CanPieceMoveHere(board, x, y, kingx, kingy, NULL, NULL))
				{
					return 0;
				}
			}
		}

		return 1;
	}	
}