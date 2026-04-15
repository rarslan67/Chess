#include "Chess.h"
#include <cmath>
#include <algorithm>

Color pieceColor(Piece p) {
    if (p == EMPTY) return NONE;
    return (p <= wK) ? WHITE : BLACK;
}

Color opponent(Color c) { return (c == WHITE) ? BLACK : WHITE; }
bool inBounds(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }

void copyBoard(const Board src, Board dst) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            dst[r][c] = src[r][c];
}

GameState initializeBoard() {
    GameState gs;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            gs.board[r][c] = EMPTY;

    Piece backRank[8] = { wR, wN, wB, wQ, wK, wB, wN, wR };
    Piece blackRank[8] = { bR, bN, bB, bQ, bK, bB, bN, bR };
    for (int c = 0; c < 8; c++) {
        gs.board[7][c] = backRank[c];
        gs.board[6][c] = wP;
        gs.board[0][c] = blackRank[c];
        gs.board[1][c] = bP;
    }
    gs.turn = WHITE;
    gs.gameOver = false;
    gs.whiteInCheck = false;
    gs.blackInCheck = false;
    gs.enPassantSq = std::nullopt;
    gs.castle = { true, true, true, true };
    return gs;
}

std::vector<Move> getRookMoves(const Board b, int row, int col, Color color) {
    std::vector<Move> moves;
    int dirs[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
    for (auto& d : dirs) {
        int r = row + d[0], c = col + d[1];
        while (inBounds(r, c)) {
            if (b[r][c] == EMPTY) { moves.push_back({ {row,col},{r,c} }); }
            else { if (pieceColor(b[r][c]) != color) moves.push_back({ {row,col},{r,c} }); break; }
            r += d[0]; c += d[1];
        }
    }
    return moves;
}

std::vector<Move> getBishopMoves(const Board b, int row, int col, Color color) {
    std::vector<Move> moves;
    int dirs[4][2] = { {1,1},{1,-1},{-1,1},{-1,-1} };
    for (auto& d : dirs) {
        int r = row + d[0], c = col + d[1];
        while (inBounds(r, c)) {
            if (b[r][c] == EMPTY) { moves.push_back({ {row,col},{r,c} }); }
            else { if (pieceColor(b[r][c]) != color) moves.push_back({ {row,col},{r,c} }); break; }
            r += d[0]; c += d[1];
        }
    }
    return moves;
}

std::vector<Move> getQueenMoves(const Board b, int row, int col, Color color) {
    auto r = getRookMoves(b, row, col, color);
    auto bi = getBishopMoves(b, row, col, color);
    r.insert(r.end(), bi.begin(), bi.end());
    return r;
}

std::vector<Move> getKnightMoves(const Board b, int row, int col, Color color) {
    std::vector<Move> moves;
    int jumps[8][2] = { {-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1} };
    for (auto& j : jumps) {
        int r = row + j[0], c = col + j[1];
        if (inBounds(r, c) && pieceColor(b[r][c]) != color)
            moves.push_back({ {row,col},{r,c} });
    }
    return moves;
}

std::vector<Move> getPawnMoves(const Board b, int row, int col, Color color,
    const std::optional<Square>& epSq) {
    std::vector<Move> moves;
    int dir = (color == WHITE) ? -1 : 1;
    int startR = (color == WHITE) ? 6 : 1;
    int promR = (color == WHITE) ? 0 : 7;
    int r1 = row + dir;
    if (!inBounds(r1, col)) return moves;

    auto addPromo = [&](int tr, int tc, bool isEP = false) {
        Piece promos[4] = {
            (color == WHITE ? wQ : bQ),(color == WHITE ? wR : bR),
            (color == WHITE ? wB : bB),(color == WHITE ? wN : bN)
        };
        for (Piece p : promos) { Move m = { {row,col},{tr,tc},p }; m.enPassant = isEP; moves.push_back(m); }
    };

    if (b[r1][col] == EMPTY) {
        if (r1 == promR) addPromo(r1, col);
        else {
            moves.push_back({ {row,col},{r1,col} });
            if (row == startR && b[r1 + dir][col] == EMPTY)
                moves.push_back({ {row,col},{r1 + dir,col} });
        }
    }

    for (int dc : { -1, 1 }) {
        int c = col + dc;
        if (!inBounds(r1, c)) continue;
        bool normalCap = (b[r1][c] != EMPTY && pieceColor(b[r1][c]) != color);
        bool epCap = (epSq && epSq->r == r1 && epSq->c == c);
        if (normalCap || epCap) {
            if (r1 == promR) addPromo(r1, c, epCap && !normalCap);
            else { Move m = { {row,col},{r1,c} }; m.enPassant = (epCap && !normalCap); moves.push_back(m); }
        }
    }
    return moves;
}

bool isSquareAttacked(const Board b, int row, int col, Color byColor,
    const std::optional<Square>& ep);

std::vector<Move> getKingMoves(const Board b, int row, int col, Color color,
    const CastleRights& cr, const std::optional<Square>& ep) {
    std::vector<Move> moves;
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            if (!dr && !dc) continue;
            int r = row + dr, c = col + dc;
            if (inBounds(r, c) && pieceColor(b[r][c]) != color)
                moves.push_back({ {row,col},{r,c} });
        }

    Color opp = opponent(color);
    int backRow = (color == WHITE) ? 7 : 0;
    bool canK = (color == WHITE) ? cr.wK : cr.bK;
    bool canQ = (color == WHITE) ? cr.wQ : cr.bQ;

    if (row == backRow && col == 4) {
        if (canK &&
            b[backRow][5] == EMPTY && b[backRow][6] == EMPTY &&
            pieceColor(b[backRow][7]) == color &&
            !isSquareAttacked(b, backRow, 4, opp, ep) &&
            !isSquareAttacked(b, backRow, 5, opp, ep) &&
            !isSquareAttacked(b, backRow, 6, opp, ep)) {
            Move m = { {row,col},{backRow,6} }; m.castleK = true; moves.push_back(m);
        }
        if (canQ &&
            b[backRow][3] == EMPTY && b[backRow][2] == EMPTY && b[backRow][1] == EMPTY &&
            pieceColor(b[backRow][0]) == color &&
            !isSquareAttacked(b, backRow, 4, opp, ep) &&
            !isSquareAttacked(b, backRow, 3, opp, ep) &&
            !isSquareAttacked(b, backRow, 2, opp, ep)) {
            Move m = { {row,col},{backRow,2} }; m.castleQ = true; moves.push_back(m);
        }
    }
    return moves;
}

std::vector<Move> getPseudoMoves(const Board b, int row, int col, Color color,
    const std::optional<Square>& ep, const CastleRights& cr) {
    Piece p = b[row][col];
    if (p == wP || p == bP) return getPawnMoves(b, row, col, color, ep);
    if (p == wR || p == bR) return getRookMoves(b, row, col, color);
    if (p == wB || p == bB) return getBishopMoves(b, row, col, color);
    if (p == wQ || p == bQ) return getQueenMoves(b, row, col, color);
    if (p == wN || p == bN) return getKnightMoves(b, row, col, color);
    if (p == wK || p == bK) return getKingMoves(b, row, col, color, cr, ep);
    return {};
}

Square findKingPosition(const Board b, Color color) {
    Piece king = (color == WHITE) ? wK : bK;
    for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) if (b[r][c] == king) return { r,c };
    return { -1,-1 };
}

bool isSquareAttacked(const Board b, int row, int col, Color byColor,
    const std::optional<Square>& ep) {
    CastleRights none = { false,false,false,false };
    for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) {
        if (b[r][c] == EMPTY || pieceColor(b[r][c]) != byColor) continue;
        auto moves = getPseudoMoves(b, r, c, byColor, ep, none);
        for (auto& m : moves) if (m.to.r == row && m.to.c == col) return true;
    }
    return false;
}

bool isKingInCheck(const Board b, Color color, const std::optional<Square>& ep) {
    Square kp = findKingPosition(b, color);
    if (!kp.valid()) return false;
    return isSquareAttacked(b, kp.r, kp.c, opponent(color), ep);
}

void simulateMove(const Board src, Board dst, const Move& m) {
    copyBoard(src, dst);
    Piece piece = dst[m.from.r][m.from.c];
    dst[m.to.r][m.to.c] = piece;
    dst[m.from.r][m.from.c] = EMPTY;
    if (m.enPassant) dst[m.from.r][m.to.c] = EMPTY;
    if (m.castleK) { dst[m.to.r][5] = dst[m.to.r][7]; dst[m.to.r][7] = EMPTY; }
    if (m.castleQ) { dst[m.to.r][3] = dst[m.to.r][0]; dst[m.to.r][0] = EMPTY; }
    if (m.promotion != EMPTY) dst[m.to.r][m.to.c] = m.promotion;
}

bool isLegalMove(const Board b, const Move& m, Color color, const std::optional<Square>& ep) {
    Board nb; simulateMove(b, nb, m);
    return !isKingInCheck(nb, color, std::nullopt);
}

std::vector<Move> filterLegalMoves(const Board b, int r, int c, Color color,
    const std::optional<Square>& ep, const CastleRights& cr) {
    auto pseudo = getPseudoMoves(b, r, c, color, ep, cr);
    std::vector<Move> legal;
    for (auto& m : pseudo) if (isLegalMove(b, m, color, ep)) legal.push_back(m);
    return legal;
}

std::vector<Move> getAllPossibleMoves(const Board b, Color color,
    const std::optional<Square>& ep, const CastleRights& cr) {
    std::vector<Move> all;
    for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) {
        if (b[r][c] == EMPTY || pieceColor(b[r][c]) != color) continue;
        auto mv = filterLegalMoves(b, r, c, color, ep, cr);
        all.insert(all.end(), mv.begin(), mv.end());
    }
    return all;
}

bool isCheckmate(const Board b, Color color, const std::optional<Square>& ep, const CastleRights& cr) {
    return isKingInCheck(b, color, ep) && getAllPossibleMoves(b, color, ep, cr).empty();
}

bool isStalemate(const Board b, Color color, const std::optional<Square>& ep, const CastleRights& cr) {
    return !isKingInCheck(b, color, ep) && getAllPossibleMoves(b, color, ep, cr).empty();
}

void makeMove(GameState& gs, const Move& m, std::vector<HistoryEntry>& history) {
    HistoryEntry e;
    copyBoard(gs.board, e.board);
    e.turn = gs.turn; e.castle = gs.castle; e.enPassantSq = gs.enPassantSq;
    history.push_back(e);

    Piece piece = gs.board[m.from.r][m.from.c];
    Piece captured = gs.board[m.to.r][m.to.c];
    if (m.enPassant)
        captured = (piece == wP) ? bP : wP;

    simulateMove(gs.board, gs.board, m);

    gs.enPassantSq = std::nullopt;
    if ((piece == wP || piece == bP) && std::abs(m.to.r - m.from.r) == 2)
        gs.enPassantSq = Square{ (m.from.r + m.to.r) / 2, m.to.c };

    if (piece == wK) { gs.castle.wK = false; gs.castle.wQ = false; }
    if (piece == bK) { gs.castle.bK = false; gs.castle.bQ = false; }
    if (piece == wR) { if (m.from.c == 0) gs.castle.wQ = false; if (m.from.c == 7) gs.castle.wK = false; }
    if (piece == bR) { if (m.from.c == 0) gs.castle.bQ = false; if (m.from.c == 7) gs.castle.bK = false; }

    if (captured == wR && m.to.r == 7 && m.to.c == 0) gs.castle.wQ = false;
    if (captured == wR && m.to.r == 7 && m.to.c == 7) gs.castle.wK = false;
    if (captured == bR && m.to.r == 0 && m.to.c == 0) gs.castle.bQ = false;
    if (captured == bR && m.to.r == 0 && m.to.c == 7) gs.castle.bK = false;

    gs.turn = opponent(gs.turn);
    gs.whiteInCheck = isKingInCheck(gs.board, WHITE, gs.enPassantSq);
    gs.blackInCheck = isKingInCheck(gs.board, BLACK, gs.enPassantSq);
}

void undoMove(GameState& gs, std::vector<HistoryEntry>& history) {
    if (history.empty()) return;
    auto& e = history.back();
    copyBoard(e.board, gs.board);
    gs.turn = e.turn; gs.castle = e.castle; gs.enPassantSq = e.enPassantSq;
    gs.gameOver = false;
    gs.whiteInCheck = isKingInCheck(gs.board, WHITE, gs.enPassantSq);
    gs.blackInCheck = isKingInCheck(gs.board, BLACK, gs.enPassantSq);
    history.pop_back();
}
