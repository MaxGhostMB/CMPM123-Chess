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
        char n = bit->gameTag();
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

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    // FENtoBoard("5k2/8/8/3q4/8/8/5PPP/7K");

    stateString();

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)
    int index = 0;
    ChessPiece piece;
    int player;

    // for future implentation
    std::string board_info;
    int active_player;
    std::string castling_rights;
    std::string en_passant_rights;

    if (fen.find(' ')) {
        size_t first_space = fen.find(' ');
        std::string placement = fen.substr(0, first_space);

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
    for (char f : fen) {
        // so that any FEN string is allowed 
        if (f == ' ') {
            break;
        }

        int placement = index ^ 56; // flip board to start from whites POV
        int x = placement % 8;
        int y = placement / 8;
        
        // get correct placement
        ChessSquare* square = _grid->getSquare(x, y);

        // what is the color
        if (isupper(f)) {
            player = 0;
        } else {
            player = 1;
        }

        // what is the piece
        if (f == 'r' || f == 'R') {
            piece = Rook;
        }
        if (f == 'b' || f == 'B') {
            piece = Bishop;
        }
        if (f == 'n' || f == 'N') {
            piece = Knight;
        }
        if (f == 'q' || f == 'Q') {
            piece = Queen;
        }
        if (f == 'k' || f == 'K') {
            piece = King;
        }
        if (f == 'p' || f == 'P') {
            piece = Pawn;
        }
        // next line
        if (f == '/') {
            continue;
        }
        // how many empty spaces 
        if (atoi(&f)) {
            for (int i = 0; i < atoi(&f); i++) {
                placement = index ^ 56;
                square = _grid->getSquareByIndex(placement);
                square->setBit(nullptr);
                index += 1;
            }
            continue;
        }
        // place pieces 
        Bit* placed = PieceForPlayer((player), piece);
        placed->setPosition(square->getPosition());
        placed->setGameTag(f);
        square->setBit(placed);
        index += 1;
    }
    std::cout << "Castling: " << castling_rights << " En Pasant: " << en_passant_rights << " Remaining Board_info: " << board_info << std::endl;
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
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
    return true;
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
