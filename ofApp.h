#pragma once
#include "ofMain.h"
#include "Chess.h"
#include <array>
#include <vector>
#include <optional>
#include <string>

static constexpr int BOARD_ORIGIN_X = 60;
static constexpr int BOARD_ORIGIN_Y = 60;
static constexpr int SQ_SIZE = 72;
static constexpr int BOARD_PX = SQ_SIZE * 8;
/// RGB distance (0–441); higher removes more “similar” pixels. Tune if edges look bitten off.
static constexpr float PIECE_BG_KEY_DISTANCE = 48.f;

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void mousePressed(int x, int y, int button) override;
    void keyPressed(int key) override;

private:
    GameState gs;
    std::vector<HistoryEntry> history;
    std::optional<Square> selected;
    std::vector<Move> legalMoves;
    bool showPromoModal = false;
    Square promoFrom, promoTo;
    bool promoEP = false;
    Color promoColor;
    std::optional<std::pair<Square, Square>> lastMove;
    std::string statusMsg;
    ofTrueTypeFont pieceFont;
    ofTrueTypeFont uiFont;
    bool usingFallbackFont = false;
    std::array<ofImage, 13> pieceImages{};
    bool pieceImagesReady = false;
    /// If true, tries to make a uniform corner color transparent (flat photo backgrounds). Set false for PNGs that already have alpha.
    bool stripSolidPieceBackground = true;

    void drawBoard();
    void drawCoordinates();
    void drawPieces();
    void drawHighlights();
    void drawStatusBar();
    void drawPromoModal();
    Square pixelToSquare(int x, int y);
    ofPoint squareToPixel(int r, int c);
    void handleSquareClick(Square sq);
    void applyMove(const Move& m);
    void updateStatus();
    void resetSelection();
    void loadPieceImages();
    static void stripSolidBackgroundFromCorners(ofImage& img, float maxRgbDist);
    void drawPieceGraphic(Piece p, float x, float y, float w, float h);
    std::string pieceUnicode(Piece p);
    std::string pieceLetter(Piece p);
    ofColor lightSquareColor();
    ofColor darkSquareColor();
    ofColor selectedColor();
    ofColor legalDotColor();
    ofColor lastMoveColor();
    ofColor checkColor();
};
