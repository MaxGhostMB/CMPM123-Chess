#include "Chess.h"
#include "MagicBitboards.h"
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
        notation = bit->gameTag() < 128  ? wpieces[bit->gameTag()] : bpieces[bit->gameTag() - 128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == WHITE ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    Player* owner = getPlayerAt(playerNumber == WHITE ? 0 : 1);
    // owner->setPlayerNumber(playerNumber);
    bit->setOwner(owner);
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _currentplayer = WHITE;
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;
    
    initMagicBitboards();
    getKingmoves();
    getKnightmoves();
    getPawnmoves();

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    
    
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    // FENtoBoard("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R");

    std::string _currentstate = stateString();
    moves = generateAllCurrentMoves(_currentstate, _currentplayer);

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

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
    int active_player = WHITE;
    std::string castling_rights;
    std::string en_passant_rights;

    std::string placement = fen;

    if (fen.find(' ') != std::string::npos) {
        size_t first_space = fen.find(' ');
        placement = fen.substr(0, first_space);

        std::istringstream iss(fen.substr(first_space + 1));
        std::string turn, castling, enpassant, halfmove, fullmove;

        iss >> turn >> castling >> enpassant >> halfmove >> fullmove;

        // active player
        active_player = (turn == "w") ? WHITE : BLACK;
        // castling rights
        castling_rights = (castling == "-") ? "-" : castling;
        // en passant
        en_passant_rights = (enpassant == "-") ? "-" : enpassant;
    } 

    for (char f : placement) {
        if (f == '/') {
            continue;
        }

        int placement_ = index ^ 56; // flip board to start from whites POV
        int x = placement_ % 8;
        int y = placement_ / 8;

        uint64_t mask = 1ULL << placement_;
        
        // get correct placement
        ChessSquare* square = _grid->getSquare(x, y);

        // how many empty spaces 
        if (isdigit(f)) {
            int count = f - '0';
            for (int i = 0; i < count; i++) {
                placement_ = index ^ 56;
                square = _grid->getSquareByIndex(placement_);
                square->setBit(nullptr);
                index += 1;
            }
            continue;
        }

        // what is the color
        if (isupper(f)) {
            player = WHITE;
        } else {
            player = BLACK;
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
        // auto print = (player == WHITE) ? piece : (piece * BLACK);
        placed->setGameTag(player == WHITE ? piece : (piece + 128));
        square->setBit(placed);

        // set bit boards 
        this->ChessBoard[BoardIndex(piece, player)] |= mask;

        // next index on the board 
        index += 1;
    }
    _currentplayer = active_player;
    std::string _currentstate = stateString();
    moves = generateAllCurrentMoves(_currentstate, active_player);
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

void Chess::generateKingmoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares, int player) {
    kingBoard.forEachBit([&](int fromSquare) {
        uint64_t moves_ = _Kingmoves[fromSquare].getData();
        int arr_i = player == WHITE ? 0 : 1;

        if (!Kingsmoved[arr_i]) {
            uint64_t KingSideMask = (player == WHITE) ? ((1ULL << 6) | (1ULL << 5)) : ((1ULL << 61) | (1ULL << 62));
            uint64_t QueenSideMask = (player == WHITE) ? ((1ULL << 1) | (1ULL << 2) | (1ULL << 3)) : ((1ULL << 57) | (1ULL << 58) | (1ULL << 59));
            
            bool KingSideCastle = (emptySquares & KingSideMask) == KingSideMask;
            bool QueenSideCastle = (emptySquares & QueenSideMask) == QueenSideMask;

            if (QueenSideCastle && !Rooksmoved[arr_i]) {
                int toSquare = (player == WHITE) ? 2 : 58;
                moves.emplace_back(fromSquare, toSquare, King);
            }
            if (KingSideCastle && !Rooksmoved[(arr_i) + 2]) {
                int toSquare = (player == WHITE) ? 6 : 62;
                moves.emplace_back(fromSquare, toSquare, King);
            }
        }
        BitboardElement moveBitboard = BitboardElement(_Kingmoves[fromSquare].getData() & emptySquares);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

// pawn moves
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
                int BtoSq = WtoSq ^ 56; // flips board to produce black moves 
                WhiteMoveBoard |= (1ULL << WtoSq);
                BlackMoveBoard |= (1ULL << BtoSq);
            }
        }
        BitboardElement _WMove(WhiteMoveBoard);
        _WhitePawnmoves[sq] = _WMove;
        BitboardElement _BMove(BlackMoveBoard);
        _BlackPawnmoves[sq ^ 56] = _BMove;
    }
}

void Chess::generateWhitePawnmoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemySquares) {
    pawnBoard.forEachBit([&](int fromSquare) {
        uint64_t forwardMask  = _WhitePawnmoves[fromSquare].getData(); 
        uint64_t diagonalMask = horizontalNeighbors(forwardMask);

        uint64_t forwardMoves  = forwardMask  & emptySquares;
        uint64_t AlowedDiagonal = diagonalMask & enemySquares;
       
        // getting our en Passant move if one exists
        uint64_t enPassant = (enPassantSquare != -1) ? 1ULL << enPassantSquare : 0ULL;
        // does it overlap with 
        AlowedDiagonal |= (enPassant & diagonalMask);

        BitboardElement moveBitboard = BitboardElement(forwardMoves | AlowedDiagonal);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Pawn);
        });
    });
}

void Chess::generateBlackPawnmoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemySquares) {
    pawnBoard.forEachBit([&](int fromSquare) {
        uint64_t forwardMask  = _BlackPawnmoves[fromSquare].getData(); 
        uint64_t diagonalMask = horizontalNeighbors(forwardMask);

        uint64_t forwardMoves  = forwardMask  & emptySquares;
        uint64_t AlowedDiagonal = diagonalMask & enemySquares;
        
        uint64_t enPassant = (enPassantSquare != -1) ? (1ULL << enPassantSquare) : 0ULL;

        AlowedDiagonal |= (enPassant & diagonalMask);
         
        BitboardElement moveBitboard = BitboardElement(forwardMoves | AlowedDiagonal);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Pawn);
        });
    });
}

void Chess::generateBishopmoves(std::vector<BitMove>& moves, BitboardElement pieceboard, uint64_t occupied, uint64_t friendlies) {
    pieceboard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(getBishopAttacks(fromSquare, occupied) & friendlies);
        // moveBitboard.printBitboard();
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Bishop);
        });
    });
}

void Chess::generateRookmoves(std::vector<BitMove>& moves, BitboardElement pieceboard, uint64_t occupied, uint64_t friendlies) {
    pieceboard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(getRookAttacks(fromSquare, occupied) & friendlies);
        // moveBitboard.printBitboard();
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Rook);
        });
    });
}

void Chess::generateQueenmoves(std::vector<BitMove>& moves, BitboardElement pieceboard, uint64_t occupied, uint64_t friendlies) {
    pieceboard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(getQueenAttacks(fromSquare, occupied) & friendlies);
        // moveBitboard.printBitboard();
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Queen);
        });
    });
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* Square_dst = dynamic_cast<ChessSquare*>(&dst);
    ChessSquare* Square_src = dynamic_cast<ChessSquare*>(&src);
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
    int piece = bit.gameTag() < 128 ? bit.gameTag() : bit.gameTag() - 128;
    int num = (bit.getOwner()->playerNumber() == 0 ? WHITE : BLACK);
    makeMove(srcIndex, dstIndex, (ChessPiece) piece, num);

    endTurn();
}

void Chess::endTurn() {
    std::string current_state = stateString();
    if (_currentplayer == WHITE) {
        moves = generateAllCurrentMoves(current_state, BLACK);
        _currentplayer = BLACK;
    } else {
        moves = generateAllCurrentMoves(current_state, WHITE);
        _currentplayer = WHITE;
    }
    // std::cout << "size: " << moves.size() << std::endl;
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
    const int enemy = (player == WHITE) ? BLACK : WHITE;
    const int idx = (player == WHITE) ? 0 : 1;

    uint64_t pieceBoard = ChessBoard[BoardIndex(piece, player)].getData();
    uint64_t moveMask = (1ULL << from) | (1ULL << to);

    // Castling
    if (piece == King && !Kingsmoved[idx]) {
        Kingsmoved[idx] = true;

        bool kingCastled  = (to == from + 2);
        bool queenCastled = (to == from - 2);

        auto moveRook = [&](int rFromWhite, int rFromBlack,
                            int rToWhite,   int rToBlack,
                            int rookIndexFlag) 
        {
            int rFrom = (player == WHITE) ? rFromWhite : rFromBlack;
            int rTo   = (player == WHITE) ? rToWhite   : rToBlack;

            // clear rook's old square
            _grid->getSquareByIndex(rFrom)->setBit(nullptr);

            // update rook bitboard
            uint64_t rookMoveMask = (1ULL << rFrom) | (1ULL << rTo);
            ChessBoard[BoardIndex(Rook, player)] ^= rookMoveMask;

            // set rook on new square
            ChessSquare *newSq = _grid->getSquareByIndex(rTo);
            Bit *rookPiece = PieceForPlayer(player, Rook);

            rookPiece->setPosition(newSq->getPosition());
            rookPiece->setGameTag(player == WHITE ? Rook : Rook * BLACK);
            newSq->setBit(rookPiece);

            // update rook-moved flags
            Rooksmoved[rookIndexFlag] = true;
        };

        if (kingCastled)
            moveRook(7, 63, 5, 61, idx + 2);

        if (queenCastled)
            moveRook(0, 56, 3, 59, idx);
    }

    if (piece == Rook) {
        Rooksmoved[idx]     = (player == WHITE ? from == 0  : from == 56);
        Rooksmoved[idx + 2] = (player == WHITE ? from == 7  : from == 63);
    }

    // En Passant 
    bool isEnPassantCapture = (to == enPassantSquare);

    // reset for next turn
    enPassantSquare = -1;

    // double pawn push sets new en passant target square
    if (piece == Pawn) {
        bool doublePush = std::abs(from - to) == 16;
        if (doublePush)
            enPassantSquare = (player == WHITE) ? from + 8 : from - 8;
    }

    pieceBoard ^= moveMask;
    ChessBoard[BoardIndex(piece, player)].setData(pieceBoard);

    // normal captures (remove any enemy piece on 'to')
    uint64_t toMask = ~(1ULL << to);

    ChessBoard[BoardIndex(Pawn,   enemy)] &= toMask;
    ChessBoard[BoardIndex(Knight, enemy)] &= toMask;
    ChessBoard[BoardIndex(Rook,   enemy)] &= toMask;
    ChessBoard[BoardIndex(Bishop, enemy)] &= toMask;
    ChessBoard[BoardIndex(Queen,  enemy)] &= toMask;

    // en passant capture
    if (isEnPassantCapture) {
        int capturedPawnSq = (player == WHITE) ? to - 8 : to + 8;

        ChessBoard[BoardIndex(Pawn, enemy)] &= ~(1ULL << capturedPawnSq);

        ChessSquare* capturedSq = _grid->getSquareByIndex(capturedPawnSq);
        capturedSq->setBit(nullptr);
    }
    // PrintChessBoards();
}

std::vector<BitMove> Chess::generateAllCurrentMoves(std::string& state, int player) {
    // clear moves 
    ClearChessState();
    std::vector<BitMove> newMoves;

    for (int i = 0; i < 64; i++) {
        char bit_ = state[i];
        ChessPiece piece;
        int color;

        if (isupper(bit_)) {
            color = WHITE;
        } else {
            color = BLACK;
        }
        bit_ = toupper(bit_);
        switch(bit_) {
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
                piece = ChessPiece::NoPiece;
        }
        ChessState[BoardIndex(piece, color)] |= (1ULL << i);
    }
    // PrintChessBoards();

    int enemy = (player == WHITE) ? BLACK : WHITE;

    // genrate boards
    uint64_t friendlySqrs = ~( ChessState[BoardIndex(Pawn, player)].getData() | ChessState[BoardIndex(Rook, player)].getData() 
                             | ChessState[BoardIndex(King, player)].getData() | ChessState[BoardIndex(Knight, player)].getData() 
                             | ChessState[BoardIndex(Bishop, player)].getData() | ChessState[BoardIndex(Queen, player)].getData());

    uint64_t enemySqrs = ( ChessState[BoardIndex(Pawn, enemy)].getData() | ChessState[BoardIndex(Rook, enemy)].getData() 
                         | ChessState[BoardIndex(King, enemy)].getData() | ChessState[BoardIndex(Knight, enemy)].getData() 
                         | ChessState[BoardIndex(Bishop, enemy)].getData() | ChessState[BoardIndex(Queen, enemy)].getData());
    
    BitboardElement enemy_(enemySqrs);
    // enemy_.printBitboard();
    
    uint64_t emptySqrs = ~((~friendlySqrs )| enemySqrs);

    if (player == WHITE) {
        generateWhitePawnmoves(newMoves, ChessState[WhitePawn], emptySqrs, enemySqrs);
    }
    if (player == BLACK) {
        generateBlackPawnmoves(newMoves, ChessState[BlackPawn], emptySqrs, enemySqrs);
    }

    generateKingmoves(newMoves, ChessState[BoardIndex(King, player)], friendlySqrs, player);
    generateKnightmoves(newMoves, ChessState[BoardIndex(Knight, player)], friendlySqrs);
    generateBishopmoves(newMoves, ChessState[BoardIndex(Bishop, player)], ~emptySqrs, friendlySqrs);
    generateRookmoves(newMoves, ChessState[BoardIndex(Rook, player)], ~emptySqrs, friendlySqrs);
    generateQueenmoves(newMoves, ChessState[BoardIndex(Queen, player)], ~emptySqrs, friendlySqrs);

    // int i = 1;
    // for (auto move_ : newMoves) {
    //     std::cout << "Move # " << i << " from: " << (int) move_.from << " to: " << (int) move_.to << " with: " << (int) move_.piece << std::endl;
    //     i++;
    // }
    return newMoves;
}

int Chess::BoardIndex(ChessPiece piece, int player) {
    if (piece == NoPiece) {
        return 12;
    }
    //if white returns 0-5 
    if (player == WHITE) {
        return (piece) - 1;
    } else { // if black return 6-11
        return (piece) + 5;
    }
}

int Chess::ArrIndex(int player) {
    if (player == WHITE) {
        return 0;
    } else {
        return 1;
    }
}

// debug function
void Chess::PrintChessBoards() {
    for (int i = 0; i < 12; i++) {
        ChessState[i].printBitboard();
    }
}

void Chess::ClearChessBoards() {
    for (int i = 0; i < 12; i++) {
        ChessBoard[i].setData(0ULL);
    }
}

void Chess::ClearChessState() {
    for (int i = 0; i < 12; i++) {
        ChessState[i].setData(0ULL);
    }
}

// for pawn captures 
uint64_t Chess::horizontalNeighbors(uint64_t bb) {
    uint64_t east  = (bb & ~FILE_H) << 1;
    uint64_t west  = (bb & ~FILE_A) >> 1;
    return east | west;
}


// AI 
void Chess::updateAI() 
{
    // std::cout << "hey\n";
    int bestVal = negInfite;
    BitMove bestMove;
    std::string state = stateString();

    std::vector<BitMove> moves_ = generateAllCurrentMoves(state, _currentplayer);
    // Traverse all cells, evaluate minimax function for all empty cells
    for (auto move : moves_) {
        // std::cout << "hey\n";
        char olddstPce = state[move.to];
        char srcPce = state[move.from];

        state[move.to] = srcPce;
        state[move.from] = '0';
        
        int moveVal = -negamax(state, 5, negInfite, posInfite, WHITE);
        // Undo the move
        state[move.to] = olddstPce;
        state[move.from] = srcPce;

        // If the value of the current move is more than the best value, update best
        if (moveVal > bestVal) {
            bestMove = move;
            bestVal = moveVal;
        }
    }

    std::cout << "from: " << (int) bestMove.from << " to: " << (int) bestMove.to << " with: " << (int) bestMove.piece << std::endl;
   // if (bestMove) {
		// TODO: Chess Square dropBitAtPoint
        int fromint = bestMove.from;
        int toint = bestMove.to;
		auto fromSquare = _grid->getSquareByIndex(fromint);
		auto toSquare = _grid->getSquareByIndex(toint);

		auto fromBit = fromSquare->bit();
		auto toPosition = toSquare->getPosition();

		toSquare->dropBitAtPoint(fromBit, toPosition);
		fromSquare->setBit(nullptr);
		bitMovedFromTo(*fromBit, *fromSquare, *toSquare);
	// }
}

int Chess::negamax(std::string& state, int depth, int alpha, int beta, int playerColor) 
{
    if (depth == 0){
        return evaluateBoard(state) * playerColor; 
    }
    
    std::vector<BitMove> moves_ = generateAllCurrentMoves(state, playerColor);

    int bestVal = -1000; // Min value

    for (auto move : moves_) {
        char olddstPce = state[move.to];
        char srcPce = state[move.from];

        state[move.to] = srcPce;
        state[move.from] = '0';
        
        int moveVal = -negamax(state, depth - 1, -beta, -alpha, -playerColor);
        // Undo the move
        state[move.to] = olddstPce;
        state[move.from] = srcPce;

        // If the value of the current move is more than the best value, update best
        if (moveVal > bestVal) {
            bestVal = moveVal;
        }
    }

    return bestVal;
}

int Chess::evaluateBoard(std::string state) {
    int values[128];
    values['P'] = 100;
    values['N'] = 300;
    values['B'] = 400;
    values['R'] = 500;
    values['Q'] = 900;
    values['K'] = 2000;
    values['p'] = -100;
    values['n'] = -300;
    values['b'] = -400;
    values['r'] = -500;
    values['q'] = -900;
    values['k'] = -2000;
    values['0'] = 0;

    int boardval = 0;

    for (int i = 0; i < 64; i++) {
        char pice = state[i];
        boardval += values[pice];
    }
    return boardval;
}
