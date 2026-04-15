#pragma once
#include <vector>
#include <optional>
#include <string>

enum Piece {
    EMPTY = 0,
    wP, wR, wN, wB, wQ, wK,
    bP, bR, bN, bB, bQ, bK
};

enum Color { WHITE, BLACK, NONE };

struct Square {
    int r = -1, c = -1;
    bool valid() const { return r >= 0 && c >= 0; }
    bool operator==(const Square& o) const { return r == o.r && c == o.c; }
};

struct Move {
    Square from, to;
    Piece promotion = EMPTY;
    bool enPassant = false;
    bool castleK = false;
    bool castleQ = false;
};

struct CastleRights {
    bool wK = true, wQ = true;
    bool bK = true, bQ = true;
};

using Board = Piece[8][8];

struct GameState {
    Board board;
    Color turn = WHITE;
    CastleRights castle;
    std::optional<Square> enPassantSq;
    bool gameOver = false;
    bool whiteInCheck = false;
    bool blackInCheck = false;
};

struct HistoryEntry {
    Board board;
    Color turn;
    CastleRights castle;
    std::optional<Square> enPassantSq;
};

Color pieceColor(Piece p);
Color opponent(Color c);
bool inBounds(int r, int c);
void copyBoard(const Board src, Board dst);

GameState initializeBoard();

std::vector<Move> getPawnMoves(const Board b, int r, int c, Color col, const std::optional<Square>& ep);
std::vector<Move> getRookMoves(const Board b, int r, int c, Color col);
std::vector<Move> getKnightMoves(const Board b, int r, int c, Color col);
std::vector<Move> getBishopMoves(const Board b, int r, int c, Color col);
std::vector<Move> getQueenMoves(const Board b, int r, int c, Color col);
std::vector<Move> getKingMoves(const Board b, int r, int c, Color col, const CastleRights& cr, const std::optional<Square>& ep);
std::vector<Move> getPseudoMoves(const Board b, int r, int c, Color col, const std::optional<Square>& ep, const CastleRights& cr);

Square findKingPosition(const Board b, Color col);
bool isSquareAttacked(const Board b, int r, int c, Color byColor, const std::optional<Square>& ep);
bool isKingInCheck(const Board b, Color col, const std::optional<Square>& ep);

void simulateMove(const Board src, Board dst, const Move& m);
bool isLegalMove(const Board b, const Move& m, Color col, const std::optional<Square>& ep);
std::vector<Move> filterLegalMoves(const Board b, int r, int c, Color col, const std::optional<Square>& ep, const CastleRights& cr);
std::vector<Move> getAllPossibleMoves(const Board b, Color col, const std::optional<Square>& ep, const CastleRights& cr);

bool isCheckmate(const Board b, Color col, const std::optional<Square>& ep, const CastleRights& cr);
bool isStalemate(const Board b, Color col, const std::optional<Square>& ep, const CastleRights& cr);

void makeMove(GameState& gs, const Move& m, std::vector<HistoryEntry>& history);
void undoMove(GameState& gs, std::vector<HistoryEntry>& history);
