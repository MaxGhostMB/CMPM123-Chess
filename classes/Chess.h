#pragma once

#include "Bitboard.h"
#include "Game.h"
#include "Grid.h"
#include "MagicBitboards.h"

#define WHITE 1
#define BLACK -1

#define negInfite -1000000
#define posInfite 1000000

constexpr int pieceSize = 80;

enum ChessBoardIndex
{
    WhitePawn,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,
    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void endTurn() override;

    bool gameHasAI() override { return true; }

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;
    void FENtoBoard(const std::string& fen);

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    char pieceNotation(int x, int y) const;

    // things to do:
    // set up bitboards for king, knight and pawns
    // generate their moves
    // 
    std::vector<BitMove> generateAllCurrentMoves(std::string&, int);

    void makeMove(int, int, ChessPiece, int);

    // knight
    void getKnightmoves();
    void generateKnightmoves(std::vector<BitMove>&, BitboardElement, uint64_t);
    std::vector<BitboardElement> _Knightmoves;

    // King
    void getKingmoves();
    void generateKingmoves(std::vector<BitMove>&, BitboardElement, uint64_t, int);
    std::vector<BitboardElement> _Kingmoves;

    // pawns 
    void getPawnmoves();
    void generateWhitePawnmoves(std::vector<BitMove>&, BitboardElement, uint64_t, uint64_t);
    void generateBlackPawnmoves(std::vector<BitMove>&, BitboardElement, uint64_t, uint64_t);
    std::vector<BitboardElement> _WhitePawnmoves;
    std::vector<BitboardElement> _BlackPawnmoves;

    void generateBishopmoves(std::vector<BitMove>&, BitboardElement, uint64_t, uint64_t);
    void generateRookmoves(std::vector<BitMove>&, BitboardElement, uint64_t, uint64_t);
    void generateQueenmoves(std::vector<BitMove>&, BitboardElement, uint64_t, uint64_t);
    

    std::vector<BitMove> moves;

    //board:
    BitboardElement ChessBoard[12];
    // let 0-5 be white and 6-11 be black
    BitboardElement ChessState[13];

    void ClearChessBoards();
    void ClearChessState();
    int BoardIndex(ChessPiece, int);
    int ArrIndex(int);

    //debug :)
    void PrintChessBoards();

    // Pawn helpers 
    uint64_t horizontalNeighbors(uint64_t bb);
    int enPassantSquare = -1;

    // Castling
    bool Kingsmoved[2] = {false, false};
    /* Rook: 
        queen side:
            white: [0] aka player
            black: [1]
        king side: 
            white: [2] aka player + 2
            black: [3]
    */
    bool Rooksmoved[4] = {false, false, false, false};
    uint64_t rankMask = 0x00000000000000FFULL;

    // sides of the board
    const uint64_t FILE_A = 0x0101010101010101ULL;
    const uint64_t FILE_H = 0x8080808080808080ULL;

    int _currentplayer;

    Grid* _grid;

    // AI stuff

    void updateAI() override;
    int negamax(std::string& state, int depth, int alpha, int beta, int playerColor);
    int evaluateBoard(std::string);
};
