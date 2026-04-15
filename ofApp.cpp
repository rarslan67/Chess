#include "ofApp.h"
#include <cmath>

ofColor ofApp::lightSquareColor() { return ofColor(240, 217, 181); }
ofColor ofApp::darkSquareColor() { return ofColor(181, 136, 99); }
ofColor ofApp::selectedColor() { return ofColor(100, 200, 100, 180); }
ofColor ofApp::legalDotColor() { return ofColor(0, 0, 0, 60); }
ofColor ofApp::lastMoveColor() { return ofColor(255, 210, 0, 120); }
ofColor ofApp::checkColor() { return ofColor(220, 50, 50, 200); }

std::string ofApp::pieceUnicode(Piece p) {
    switch (p) {
    case wK: return "\u2654"; case wQ: return "\u2655";
    case wR: return "\u2656"; case wB: return "\u2657";
    case wN: return "\u2658"; case wP: return "\u2659";
    case bK: return "\u265A"; case bQ: return "\u265B";
    case bR: return "\u265C"; case bB: return "\u265D";
    case bN: return "\u265E"; case bP: return "\u265F";
    default: return "";
    }
}

std::string ofApp::pieceLetter(Piece p) {
    switch (p) {
    case wP: return "P"; case wR: return "R"; case wN: return "N";
    case wB: return "B"; case wQ: return "Q"; case wK: return "K";
    case bP: return "p"; case bR: return "r"; case bN: return "n";
    case bB: return "b"; case bQ: return "q"; case bK: return "k";
    default: return "";
    }
}

void ofApp::stripSolidBackgroundFromCorners(ofImage& img, float maxRgbDist) {
    if (!img.isAllocated()) return;
    img.setImageType(OF_IMAGE_COLOR_ALPHA);
    ofPixels& px = img.getPixels();
    int w = px.getWidth(), h = px.getHeight();
    if (w < 2 || h < 2) return;

    ofColor c00 = px.getColor(0, 0);
    ofColor c10 = px.getColor(w - 1, 0);
    ofColor c01 = px.getColor(0, h - 1);
    ofColor c11 = px.getColor(w - 1, h - 1);

    // Already-transparent corners: assume asset has proper alpha; don’t key out colors.
    if (!(c00.a > 128 && c10.a > 128 && c01.a > 128 && c11.a > 128))
        return;

    float kr = (c00.r + c10.r + c01.r + c11.r) / 4.f;
    float kg = (c00.g + c10.g + c01.g + c11.g) / 4.f;
    float kb = (c00.b + c10.b + c01.b + c11.b) / 4.f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            ofColor c = px.getColor(x, y);
            float dr = c.r - kr, dg = c.g - kg, db = c.b - kb;
            float dist = std::sqrt(dr * dr + dg * dg + db * db);
            if (dist <= maxRgbDist)
                px.setColor(x, y, ofColor(0, 0, 0, 0));
        }
    }
    img.update();
}

void ofApp::loadPieceImages() {
    static const char* stem[13] = {
        nullptr,
        "wp", "wr", "wn", "wb", "wq", "wk",
        "bp", "br", "bn", "bb", "bq", "bk"
    };
    pieceImagesReady = true;
    for (int i = static_cast<int>(wP); i <= static_cast<int>(bK); ++i) {
        std::string path = std::string("pieces/") + stem[i] + ".png";
        if (!pieceImages[static_cast<size_t>(i)].load(ofToDataPath(path, true))) {
            ofLogWarning("ofApp") << "Missing piece image: bin/data/" << path << " — using font fallback";
            pieceImagesReady = false;
            break;
        }
        if (stripSolidPieceBackground)
            stripSolidBackgroundFromCorners(pieceImages[static_cast<size_t>(i)], PIECE_BG_KEY_DISTANCE);
    }
}

void ofApp::drawPieceGraphic(Piece p, float x, float y, float w, float h) {
    if (pieceImagesReady && p > EMPTY && p <= bK &&
        pieceImages[static_cast<size_t>(p)].isAllocated()) {
        const float pad = 4.f;
        ofEnableAlphaBlending();
        ofSetColor(255);
        pieceImages[static_cast<size_t>(p)].draw(x + pad, y + pad, w - 2.f * pad, h - 2.f * pad);
        return;
    }
    std::string label = usingFallbackFont ? pieceLetter(p) : pieceUnicode(p);
    bool isWhite = (pieceColor(p) == WHITE);
    float textY = y + h - 8.f;
    ofSetColor(0, 0, 0, 100);
    pieceFont.drawString(label, x + 5.f, textY + 2.f);
    ofSetColor(isWhite ? ofColor(255, 255, 255) : ofColor(15, 15, 15));
    pieceFont.drawString(label, x + 5.f, textY);
}

void ofApp::setup() {
    ofSetWindowShape(BOARD_ORIGIN_X * 2 + BOARD_PX, BOARD_ORIGIN_Y + BOARD_PX + 80);
    ofSetWindowTitle("Chess - 2 Player");
    ofSetFrameRate(60);
    ofBackground(lightSquareColor());
    ofTrueTypeFont::setGlobalDpi(72);

    bool fontLoaded = pieceFont.load(
        ofToDataPath("fonts/NotoSans-Regular.ttf", true),
        SQ_SIZE - 14, true, true, true);

    if (!fontLoaded)
        fontLoaded = pieceFont.load(
            "C:/Windows/Fonts/ArialUni.ttf",
            SQ_SIZE - 14, true, true, true);

    if (!fontLoaded) {
        pieceFont.load(OF_TTF_SANS, SQ_SIZE - 14);
        usingFallbackFont = true;
    }

    uiFont.load(OF_TTF_SANS, 14);
    loadPieceImages();
    gs = initializeBoard();
    updateStatus();
}

void ofApp::update() {}

void ofApp::draw() {
    drawBoard();
    drawHighlights();
    drawCoordinates();
    drawPieces();
    drawStatusBar();
    if (showPromoModal) drawPromoModal();
}

void ofApp::drawBoard() {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            ofSetColor(((r + c) % 2 == 0) ? lightSquareColor() : darkSquareColor());
            ofPoint tl = squareToPixel(r, c);
            ofDrawRectangle(tl.x, tl.y, SQ_SIZE, SQ_SIZE);
        }
}

void ofApp::drawHighlights() {
    if (lastMove) {
        for (auto& sq : { lastMove->first, lastMove->second }) {
            ofSetColor(lastMoveColor());
            ofPoint tl = squareToPixel(sq.r, sq.c);
            ofDrawRectangle(tl.x, tl.y, SQ_SIZE, SQ_SIZE);
        }
    }

    auto highlightKing = [&](Color col) {
        Square kp = findKingPosition(gs.board, col);
        if (kp.valid()) {
            ofSetColor(checkColor());
            ofPoint tl = squareToPixel(kp.r, kp.c);
            ofDrawRectangle(tl.x, tl.y, SQ_SIZE, SQ_SIZE);
        }
        };
    if (gs.whiteInCheck) highlightKing(WHITE);
    if (gs.blackInCheck) highlightKing(BLACK);

    if (selected) {
        ofSetColor(selectedColor());
        ofPoint tl = squareToPixel(selected->r, selected->c);
        ofDrawRectangle(tl.x, tl.y, SQ_SIZE, SQ_SIZE);

        ofSetColor(legalDotColor());
        for (auto& m : legalMoves) {
            ofPoint center = squareToPixel(m.to.r, m.to.c);
            center.x += SQ_SIZE / 2;
            center.y += SQ_SIZE / 2;
            bool isCapture = (gs.board[m.to.r][m.to.c] != EMPTY || m.enPassant);
            if (isCapture) {
                ofNoFill(); ofSetLineWidth(4);
                ofDrawCircle(center.x, center.y, SQ_SIZE / 2 - 4);
                ofFill(); ofSetLineWidth(1);
            }
            else {
                ofDrawCircle(center.x, center.y, SQ_SIZE / 6);
            }
        }
    }
}

void ofApp::drawCoordinates() {
    ofSetColor(200, 200, 200);
    const std::string files = "abcdefgh";
    const std::string ranks = "87654321";
    for (int i = 0; i < 8; i++) {
        ofPoint tl = squareToPixel(7, i);
        uiFont.drawString(std::string(1, files[i]), tl.x + SQ_SIZE / 2 - 5, tl.y + SQ_SIZE + 20);
        ofPoint tl2 = squareToPixel(i, 0);
        uiFont.drawString(std::string(1, ranks[i]), tl2.x - 22, tl2.y + SQ_SIZE / 2 + 6);
    }
}

void ofApp::drawPieces() {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            Piece p = gs.board[r][c];
            if (p == EMPTY) continue;
            ofPoint tl = squareToPixel(r, c);
            drawPieceGraphic(p, tl.x, tl.y, static_cast<float>(SQ_SIZE), static_cast<float>(SQ_SIZE));
        }
}

void ofApp::drawStatusBar() {
    int barY = BOARD_ORIGIN_Y + BOARD_PX + 8;
    ofSetColor(220, 198, 165);
    ofDrawRectangle(BOARD_ORIGIN_X, barY, BOARD_PX, 50);
    ofSetColor(220, 220, 220);
    uiFont.drawString(statusMsg, BOARD_ORIGIN_X + 12, barY + 30);
    std::string hint = "[U] Undo   [N] New game   Castle: king two steps   Promo: pick piece";
    ofSetColor(160, 160, 160);
    ofRectangle bounds = uiFont.getStringBoundingBox(hint, 0, 0);
    uiFont.drawString(hint, BOARD_ORIGIN_X + BOARD_PX - bounds.width - 12, barY + 30);
}

void ofApp::drawPromoModal() {
    ofSetColor(0, 0, 0, 160);
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());

    float boxW = 4 * SQ_SIZE + 32.0f, boxH = SQ_SIZE + 48.0f;
    float boxX = (ofGetWidth() - boxW) / 2.0f, boxY = (ofGetHeight() - boxH) / 2.0f;

    ofSetColor(50, 50, 50);
    ofDrawRectRounded(boxX, boxY, boxW, boxH, 12);
    ofSetColor(180, 180, 180); ofNoFill();
    ofDrawRectRounded(boxX, boxY, boxW, boxH, 12);
    ofFill();
    ofSetColor(200, 200, 200);
    uiFont.drawString("Promote pawn to:", boxX + 12, boxY + 22);

    Piece opts[4];
    if (promoColor == WHITE) { opts[0] = wQ; opts[1] = wR; opts[2] = wB; opts[3] = wN; }
    else { opts[0] = bQ; opts[1] = bR; opts[2] = bB; opts[3] = bN; }

    for (int i = 0; i < 4; i++) {
        float px = boxX + 16 + i * SQ_SIZE, py = boxY + 32;
        ofSetColor(80, 80, 80);
        ofDrawRectRounded(px, py, SQ_SIZE - 4, SQ_SIZE - 4, 8);
        drawPieceGraphic(opts[i], px, py, SQ_SIZE - 4.f, SQ_SIZE - 4.f);
    }
}

ofPoint ofApp::squareToPixel(int r, int c) {
    return ofPoint(BOARD_ORIGIN_X + c * SQ_SIZE, BOARD_ORIGIN_Y + r * SQ_SIZE);
}

Square ofApp::pixelToSquare(int x, int y) {
    int c = (x - BOARD_ORIGIN_X) / SQ_SIZE;
    int r = (y - BOARD_ORIGIN_Y) / SQ_SIZE;
    if (r < 0 || r > 7 || c < 0 || c > 7) return { -1,-1 };
    return { r, c };
}

void ofApp::mousePressed(int x, int y, int button) {
    if (button != 0) return;

    if (showPromoModal) {
        float boxW = 4 * SQ_SIZE + 32.0f, boxH = SQ_SIZE + 48.0f;
        float boxX = (ofGetWidth() - boxW) / 2.0f, boxY = (ofGetHeight() - boxH) / 2.0f;
        Piece opts[4];
        if (promoColor == WHITE) { opts[0] = wQ; opts[1] = wR; opts[2] = wB; opts[3] = wN; }
        else { opts[0] = bQ; opts[1] = bR; opts[2] = bB; opts[3] = bN; }
        for (int i = 0; i < 4; i++) {
            float px = boxX + 16 + i * SQ_SIZE, py = boxY + 32;
            if (x >= px && x <= px + SQ_SIZE - 4 && y >= py && y <= py + SQ_SIZE - 4) {
                auto moves = filterLegalMoves(gs.board, promoFrom.r, promoFrom.c,
                    promoColor, gs.enPassantSq, gs.castle);
                bool applied = false;
                for (auto& m : moves) {
                    if (m.to.r == promoTo.r && m.to.c == promoTo.c && m.promotion == opts[i]) {
                        applyMove(m);
                        applied = true;
                        break;
                    }
                }
                if (applied) showPromoModal = false;
                return;
            }
        }
        return;
    }

    Square sq = pixelToSquare(x, y);
    if (!sq.valid()) return;
    handleSquareClick(sq);
}

void ofApp::keyPressed(int key) {
    if (key == 'u' || key == 'U') {
        showPromoModal = false;
        undoMove(gs, history);
        lastMove = std::nullopt;
        resetSelection();
        updateStatus();
    }
    if (key == 'n' || key == 'N') {
        showPromoModal = false;
        gs = initializeBoard();
        history.clear();
        lastMove = std::nullopt;
        resetSelection();
        updateStatus();
    }
}

void ofApp::handleSquareClick(Square sq) {
    if (gs.gameOver) return;

    if (selected) {
        bool hasPromo = std::any_of(legalMoves.begin(), legalMoves.end(),
            [&](const Move& m) { return m.to.r == sq.r && m.to.c == sq.c && m.promotion != EMPTY; });

        if (hasPromo) {
            promoFrom = *selected; promoTo = sq; promoColor = gs.turn;
            showPromoModal = true; resetSelection(); return;
        }

        auto it = std::find_if(legalMoves.begin(), legalMoves.end(),
            [&](const Move& m) { return m.to.r == sq.r && m.to.c == sq.c && m.promotion == EMPTY; });

        if (it != legalMoves.end()) { applyMove(*it); return; }

        resetSelection();
    }

    Piece p = gs.board[sq.r][sq.c];
    if (p != EMPTY && pieceColor(p) == gs.turn) {
        selected = sq;
        legalMoves = filterLegalMoves(gs.board, sq.r, sq.c, gs.turn,
            gs.enPassantSq, gs.castle);
    }
}

void ofApp::applyMove(const Move& m) {
    lastMove = { {m.from, m.to} };
    makeMove(gs, m, history);
    resetSelection();
    updateStatus();
}

void ofApp::updateStatus() {
    Color c = gs.turn;
    std::string name = (c == WHITE) ? "White" : "Black";
    if (isCheckmate(gs.board, c, gs.enPassantSq, gs.castle)) {
        statusMsg = "Checkmate!  " + std::string(c == WHITE ? "Black" : "White") + " wins!";
        gs.gameOver = true;
    }
    else if (isStalemate(gs.board, c, gs.enPassantSq, gs.castle)) {
        statusMsg = "Stalemate - draw!";
        gs.gameOver = true;
    }
    else if (gs.whiteInCheck || gs.blackInCheck) {
        statusMsg = name + " is in CHECK!";
    }
    else {
        statusMsg = name + " to move";
    }
}

void ofApp::resetSelection() {
    selected = std::nullopt;
    legalMoves.clear();
}
