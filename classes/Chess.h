#pragma once

#include "Bitboard.h"
#include "Game.h"
#include "Grid.h"

constexpr int pieceSize = 80;

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
    void generateAllCurrentMoves(std::vector<BitMove>&, int);

    void makeMove(int, int, ChessPiece, int);

    // knight
    void getKnightmoves();
    void generateKnightmoves(std::vector<BitMove>&, BitboardElement, uint64_t);
    std::vector<BitboardElement> _Knightmoves;

    // King
    void getKingmoves();
    void generateKingmoves(std::vector<BitMove>&, BitboardElement, uint64_t);
    std::vector<BitboardElement> _Kingmoves;

    // pawns 
    void getPawnmoves();
    void generateWhitePawnmoves(std::vector<BitMove>&, BitboardElement, uint64_t, uint64_t);
    void generateBlackPawnmoves(std::vector<BitMove>&, BitboardElement, uint64_t, uint64_t);
    std::vector<BitboardElement> _WhitePawnmoves;
    std::vector<BitboardElement> _BlackPawnmoves;

    std::vector<BitMove> moves;

    //board:
    BitboardElement ChessBoard[12];
    // let 0-5 be white and 6-11 be black
    void ClearChessBoards();
    int BoardIndex(ChessPiece, int);

    //debug :)
    void PrintChessBoards();

    

    BitboardElement board;

    Grid* _grid;
};
