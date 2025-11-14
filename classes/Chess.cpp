#include "Chess.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        char n = bit->gameTag() < 7  ? wpieces[bit->gameTag()] : bpieces[bit->gameTag() - 6];
        notation = n;// bit->gameTag() < 128  ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;
    
    getKingmoves();
    getKnightmoves();
    getPawnmoves();

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    
    
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    generateAllCurrentMoves(moves, getCurrentPlayer()->playerNumber());
    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    int index = 0;
    ChessPiece piece;
    int player;

    uint64_t _board = 0;

    ClearChessBoards(); 

    // for future implentation
    std::string board_info;
    int active_player = 0;
    std::string castling_rights;
    std::string en_passant_rights;

    std::string placement = fen;

    if (fen.find(' ')) {
        size_t first_space = fen.find(' ');
        placement = fen.substr(0, first_space);

        std::istringstream iss(fen.substr(first_space + 1));
        std::string turn, castling, enpassant, halfmove, fullmove;

        iss >> turn >> castling >> enpassant >> halfmove >> fullmove;

        // active player
        active_player = (turn == "w") ? 0 : 1;
        // castling rights
        castling_rights = (castling == "-") ? "-" : castling;
        // en passant
        en_passant_rights = (enpassant == "-") ? "-" : enpassant;
    } 

    for (char f : placement) {
        if (f == '/') {
            continue;
        }

        int placement = index ^ 56; // flip board to start from whites POV
        int x = placement % 8;
        int y = placement / 8;

        uint64_t mask = 1ULL << placement;
        
        // get correct placement
        ChessSquare* square = _grid->getSquare(x, y);

        // how many empty spaces 
        if (isdigit(f)) {
            int count = f - '0';
            for (int i = 0; i < count; i++) {
                placement = index ^ 56;
                square = _grid->getSquareByIndex(placement);
                square->setBit(nullptr);
                index += 1;
            }
            continue;
        }

        // what is the color
        if (isupper(f)) {
            player = 0;
        } else {
            player = 1;
        }
        // what is the piece
        f = toupper(f);
        switch (f) {
            case 'R':
                piece = ChessPiece::Rook;
                break;
            case 'B':
                piece = ChessPiece::Bishop;
                break;
            case 'N':
                piece = ChessPiece::Knight;
                break;
            case 'Q':
                piece = ChessPiece::Queen;
                break;
            case 'K':
                piece = ChessPiece::King;
                break;
            case 'P':
                piece = ChessPiece::Pawn;
                break;
            default:
                break;
        }

        // place pieces 
        Bit* placed = PieceForPlayer(player, piece);
        placed->setPosition(square->getPosition());
        placed->setGameTag(player == 0 ? piece : piece + 6);
        square->setBit(placed);

        // set bit boards 
        this->ChessBoard[BoardIndex(piece, player)] |= mask;
        _board = _board | mask ;

        // next index on the board 
        index += 1;
    }
    this->board.setData(_board);
    generateAllCurrentMoves(moves, active_player);
    // PrintChessBoards();
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

// Chess Moves:
// knight
void Chess::getKnightmoves() {
    _Knightmoves.reserve(64);

    std::pair<int, int> dir[] = {
    {2, 1}, {1, 2}, {-1, 2}, {-1, -2},
    {1, -2}, {2, -1}, {-2, -1}, {-2, 1}
    };

    uint64_t MoveBoard;

    for (int sq = 0 ; sq < 64; sq++) {
        MoveBoard = 0ULL;
        int x = sq % 8;
        int y = sq / 8;
        for (auto [dx, dy] : dir) {
            int newX = x + dx;
            int newY = y + dy;

            // Check board bounds
            if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
                int toSq = newY * 8 + newX;
                MoveBoard |= (1ULL << toSq);
            }
        }
        BitboardElement _Move(MoveBoard);
        _Knightmoves[sq] = _Move;
    }
}

void Chess::generateKnightmoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares) {
    knightBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_Knightmoves[fromSquare].getData() & emptySquares);
        // moveBitboard.printBitboard();
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

// king
void Chess::getKingmoves() {
    _Kingmoves.reserve(64);

    std::pair<int, int> dir[] = {
    {1, 1}, {1, 0}, {-1, 1}, {-1, 0},
    {1, -1}, {0, -1}, {-1, -1}, {0, 1}
    };

    uint64_t MoveBoard;

    for (int sq = 0 ; sq < 64; sq++) {
        MoveBoard = 0ULL;
        int x = sq % 8;
        int y = sq / 8;
        for (auto [dx, dy] : dir) {
            int newX = x + dx;
            int newY = y + dy;

            // Check board bounds
            if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
                int toSq = newY * 8 + newX;
                MoveBoard |= (1ULL << toSq);
            }
        }
        BitboardElement _Move(MoveBoard);
        _Kingmoves[sq] = _Move;
    }
}

void Chess::generateKingmoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares) {
    kingBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_Kingmoves[fromSquare].getData() & emptySquares);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

void Chess::getPawnmoves() {
    _WhitePawnmoves.reserve(64);
    _BlackPawnmoves.reserve(64);

    int dir[] = { 1, 2 };
 

    uint64_t WhiteMoveBoard;
    uint64_t BlackMoveBoard;
    // white:
    for (int sq = 8 ; sq < 64; sq++) {
        WhiteMoveBoard = 0ULL;
        BlackMoveBoard = 0ULL;
        int x = sq % 8;
        int y = sq / 8;
       
        for (auto dy : dir) {
            if (sq > 15 && dy == 2) {
                continue;
            }
            int newY = y + dy;
            // Check board bounds
            if (newY >= 0 && newY < 8) {
                int WtoSq = newY * 8 + x;
                int BtoSq = WtoSq ^ 56; // flips board to produce 
                WhiteMoveBoard |= (1ULL << WtoSq);
                BlackMoveBoard |= (1ULL << BtoSq);
            }
        }
        BitboardElement _WMove(WhiteMoveBoard);
        _WhitePawnmoves[sq] = _WMove;
        BitboardElement _BMove(BlackMoveBoard);
        _BlackPawnmoves[sq] = _BMove;

        // if (sq < 20){
        //     std::cout << "White Board: " << std::endl;
        //     _WMove.printBitboard();
        //     std::cout << "Black Board: " << std::endl;
        //     _BMove.printBitboard();
        // }
    }
}
void Chess::generateWhitePawnmoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemySquares) {
    pawnBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_WhitePawnmoves[fromSquare].getData() & emptySquares);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Pawn);
        });
    });
}

void Chess::generateBlackPawnmoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemySquares) {
    pawnBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_BlackPawnmoves[fromSquare].getData() & emptySquares);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Pawn);
        });
    });
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber();
    int pieceColor = (bit.gameTag() < 7) ? 0 : 1;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* Square_dst = (ChessSquare *)&dst;
    ChessSquare* Square_src = (ChessSquare *)&src;
    if(Square_dst) {
        int dstIndex = Square_dst->getSquareIndex();
        int srcIndex = Square_src->getSquareIndex();
        for(auto move : moves) {
            if(move.to == dstIndex && move.from == srcIndex) {
                return true;
            }
            
        }
    }
    return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {

    ChessSquare* Square_dst = dynamic_cast<ChessSquare*>(&dst);
    ChessSquare* Square_src = dynamic_cast<ChessSquare*>(&src);

    if(!Square_dst) return;
    
    int dstIndex = Square_dst->getSquareIndex();
    int srcIndex = Square_src->getSquareIndex();
    makeMove(srcIndex, dstIndex, (ChessPiece) bit.gameTag(), bit.getOwner()->playerNumber());

    // std::cout << "Bit info: " << bit.gameTag() << " " << bit.EntitySprite << std::endl;
    endTurn();
}

void Chess::endTurn() {
    if (getCurrentPlayer()->playerNumber() == 0 ) {
        generateAllCurrentMoves(moves, 1);
    } else {
        generateAllCurrentMoves(moves, 0);
    }
    // for (int i = 0; i < moves.size(); i++) {
    //     std::cout << int(moves[i].from) << " " << int(moves[i].to) << " " << int(moves[i].piece )<< " Move\n"; 
    // }
    std::cout << "size: " << moves.size() << std::endl;
    Game::endTurn();
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        s += pieceNotation(x, y);
        // std::cout << "X: " << x << " Y: " << y << " owner: " << pieceNotation(x, y) << std::endl;
    });
    return s;
}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

void Chess::makeMove(int from, int to, ChessPiece piece, int player) {
    // std::cout << "From: " << from << " To: " << to << " Piece: " << piece << " Player: " << player << std::endl;
    // ChessBoard[BoardIndex(piece, player)].printBitboard();
    uint64_t pieceBoard = ChessBoard[BoardIndex(piece, player)].getData();
    uint64_t move = 0ULL | (1ULL << from) | (1ULL << to);

    pieceBoard ^= move;
    ChessBoard[BoardIndex(piece, player)].setData(pieceBoard);
    // ChessBoard[BoardIndex(piece, player)].printBitboard();
}

void Chess::generateAllCurrentMoves(std::vector<BitMove>& Moves, int player) {
    moves.clear();
    uint64_t emptySqrs = ~(ChessBoard[BoardIndex(Pawn, player)].getData() | ChessBoard[BoardIndex(Rook, player)].getData() 
                        | ChessBoard[BoardIndex(King, player)].getData() | ChessBoard[BoardIndex(Knight, player)].getData() 
                        |ChessBoard[BoardIndex(Bishop, player)].getData() | ChessBoard[BoardIndex(Queen, player)].getData());

    // board.printBitboard();
    // ChessBoard[BoardIndex(King, player)].printBitboard();
    // ChessBoard[BoardIndex(Knight, player)].printBitboard();
    generateKingmoves(Moves, ChessBoard[BoardIndex(King, player)], emptySqrs);
    generateKnightmoves(Moves, ChessBoard[BoardIndex(Knight, player)], emptySqrs);
}

int Chess::BoardIndex(ChessPiece piece, int player) {
    //if white returns 0-5 
    if (player == 0) {
        return (piece) - 1;
    } else { // if black return 6-11
        return (piece) + 5;
    }
}

void Chess::PrintChessBoards() {
    for (int i = 0; i < 12; i++) {
        ChessBoard[i].printBitboard();
    }
}

void Chess::ClearChessBoards() {
    for (int i = 0; i < 12; i++) {
        ChessBoard[i].setData(0ULL);
    }
}
