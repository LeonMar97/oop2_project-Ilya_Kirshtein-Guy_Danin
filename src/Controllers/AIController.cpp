#include "Controllers/AIController.h"
#include "ChessLogic/NBoard.h"

AIController::AIController(Color color, AIDifficult difficult)
    :Controller(color), m_useBook(true), m_book(&m_Openingbook.getStart()),
     m_difficulty(difficult), m_depth(difficult)
{
    srand(unsigned(time(NULL)));
}

bool AIController::turnReady()
{
    return true;
}

/*
* will return a move based on OpeningBook or MinMax algo
*/
Move AIController::playTurn()
{
    if (m_useBook)
    {
        auto move = playByBook();
        if (move.startSquare != -1)
        {
            return move;
        }
    }

    NBoard& ins = NBoard::instance();
    m_depth = m_difficulty;
    IGenerate generate;
    Move bestMove = { -1, -1 , -1, -1 ,PawnVal};

    /*
    * We generate all moves, and for each move apply minimax.
    */
    int bestValue = (m_color == WHITE) ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    std::vector<std::vector<Move>> allMoves = generate.generateAll(m_color);
    for (const auto &i : allMoves) {
        for (auto &move : i) {
            ins.move(move);
            int boardValue = minimax(m_depth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), m_color == BLACK,ins);
            ins.undo();

            if ((m_color == WHITE && boardValue > bestValue) || (m_color == BLACK && boardValue < bestValue)) {
                bestValue = boardValue;
                bestMove = move;
            }
        }
    }
	return bestMove;
}

bool AIController::isGameOver(std::vector<std::vector<Move>> all)
{
    for (const auto &i : all) 
        if (!i.empty())return false;
    return true;
}

/*
* using a tree generated by OpeningBook
* checks whether the move played by opponent has children,
* if so will play a random move from it.
* it ensures the AI will feel abit different each game.
*/
Move AIController::playByBook()
{
    Move lastMove = NBoard::instance().getLastMove();
    if (lastMove.startSquare == -1) // AI moves first.
    {
        int random = rand() % m_book->size();
        Move move = { (*m_book)[random].start, (*m_book)[random].target };
        m_book = &(*m_book)[random].children;
        return move;
    }

    Color enemyColor = m_color == White ? Black : White;
    bookMove tofind = { lastMove.startSquare, lastMove.targetSquare, enemyColor };
    auto match = std::find(m_book->begin(), m_book->end(), tofind);
    if (match == m_book->end())
    {
        m_useBook = false;
        return playTurn();
    }
    
    m_book = &(*match).children;
    if (m_book->size() <= 0)
    {
        m_useBook = false;
        return { -1, -1 };
    }
    int random = rand() % m_book->size();
    Move move = { (*m_book)[random].start, (*m_book)[random].target };
    m_book = &(*m_book)[random].children;
    return move;
}

/*
* The minmax algorithm will get a board position, and recusivly play all possible moves, 
* and on those moves will apply minmax, up to depth.
* for each move it will evaluate the board and return a value,
* overall this will return a move that has the highest value.
* 
* The algorithm also uses alpha beta pruning:
* minmax plays both black and white moves, so
* it will 'cut' and stop the recursion if it finds moves that are better for the opponent,
* it assumes the opponent will make the best move for them, and so we reduce the number 
* of positions we need to check.
* for example: if we look at 3 different moves, and on the first the evaluation favours
* the enemy, then if on the second move we find a lower evaluation, we will skip the entire recursion 
* of that move.
*/
int AIController::minimax(int depth, int alpha, int beta, bool maximizingPlayer , NBoard& ins)
{
    if (depth == 0) 
        return evaluateBoard();
    
    IGenerate generate;
    std::vector<std::vector<Move>> allMoves  = generate.generateAll(maximizingPlayer ? WHITE : BLACK);
    
    if (isGameOver(allMoves))
        depth = 1;
   

    if (maximizingPlayer) {
        int maxEval = std::numeric_limits<int>::min();
        for (const auto& i : allMoves) {
            for (const auto& move : i) {

                ins.move(move);
                int eval = minimax(depth - 1, alpha, beta, false,ins);
                ins.undo();

                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                if (beta <= alpha) {
                    return maxEval;
                }
            }
        }
        return maxEval;
    }
    else {
        int minEval = std::numeric_limits<int>::max();
        for (const auto& i : allMoves) {
            for (const auto& move : i) {

                ins.move(move);
                int eval = minimax(depth - 1, alpha, beta, true,ins);
                ins.undo();

                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                if (beta <= alpha) {
                    return minEval;
                }
            }
        }
        return minEval;
    }
}

/*
* goes through the board position, and calculates 
* a value for it.
* negative means the position favors the enemy,
* positive means the position favors the AI.
* will calculate based on Piece values and the Tables.
*/
int AIController::evaluateBoard() const
{
    NBoard& ins = NBoard::instance();
    int score = 0;
    int pieceValue = 0;
    int counter = 0;
    for (int x = 0; x < 64; x++) {
        if (ins.getPiece(x) != 0) counter++;
    }
    for (int x = 0; x < 64; x++) {
        if (ins.getPiece(x) == 0) continue;
        int piece = ins.getPiece(x);
        int color = (piece & White) > 0 ? White : Black;
        int pieceType = (piece & 0b111);
        if (piece) {
            switch (pieceType) {
            case PawnVal: {
                pieceValue = color == White ? 100 : -100;
                pieceValue += color == White ? whitePawnTable[x] : -blackPawnTable[x];
                break;
            }
            case RookVal: {
                pieceValue = color == White ? 500 : -500 ;
                pieceValue += color == White ? whiteRookTable[x] : -blackRookTable[x];
                break;
            }
            case KnightVal: {
                pieceValue = color == White ? 320 : -320;
                pieceValue += color == White ? whiteKnightTable[x] : -blackKnightTable[x];
                break;
            }
            case BishopVal: {
                pieceValue = color == White ? 330 : -330;
                pieceValue += color == White ? whiteBishopTable[x] : -blackBishopTable[x];
                break;
            }
            case QueenVal: {
                pieceValue = color == White ? 900 : -900;
                pieceValue += color == White ? whiteQueenTable[x] : -blackQueenTable[x];
                break;
            }
            case KingVal: {
                pieceValue = color == White ? 2000 : -2000;
                if(counter < 6)
                    pieceValue += color == White ? whiteKingTableEndgame[x] : -blackKingTableEndgame[x];
                else pieceValue += color == White ? whiteKingTableMidgame[x] : -blackKingTableMidgame[x];
                break;
            }
        }
        score += pieceValue;
        }
    }
    return score;
}

