#include <bits/stdc++.h>
#include "chess.hpp"

using namespace chess;

#define MATE 10000
#define INF 1000000

const int QUEEN_VAL = 900;
const int ROOK_VAL = 500;
const int BISHOP_VAL = 300;
const int KNIGHT_VAL = 300;
const int PAWN_VAL = 100;

int utility(Board& board, Movelist& moves);
int minimax(Board& board, int depth, int max_depth, int max_player_flag, int alpha, int beta, Move&final_move);
int moveScore(Board& board, const Move& move, const Move& ttMove);
int material(Board& board);

struct TTEntry {
    uint64_t hash;      // which position (Zobrist hash)
    int remaining;          // how deep was it searched
    int score;          // what score was found
    Move best_move;     // best move found (for move ordering)
    int flag;           // exact / lowerbound / upperbound
};

const int TT_SIZE = 1 << 20; // 1M entries
TTEntry tt[TT_SIZE];
std::map<chess::PieceType, int> pieceValueMap = {
    { PieceType::PAWN,   100 },
    { PieceType::KNIGHT, 320 },
    { PieceType::BISHOP, 330 },
    { PieceType::ROOK,   500 },
    { PieceType::QUEEN,  900 },
    { PieceType::KING,   20000 }
};

PieceType pieces[6] = {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN, PieceType::KING};
const int PIECE_VALUES[6] = {100, 320, 330, 500, 900, 20000};

int main() {
    std::cout.setf(std::ios::unitbuf); // auto-flush every write
    Board board;
    std::string line;

    while (std::getline(std::cin, line)) {

        // --- UCI handshake ---
        if (line == "uci") {
            std::cout << "id name FirstMoveEngine\n";
            std::cout << "id author Venkatesh\n";
            std::cout << "uciok\n";
        }

        // --- Ready check ---
        else if (line == "isready") {
            std::cout << "readyok\n";
        }

        // --- New game ---
        else if (line == "ucinewgame") {
            board = Board();
        }

        // --- Position command ---
        else if (line.starts_with("position")) {
            std::istringstream iss(line);
            std::string token;
            iss >> token; // "position"

            iss >> token;
            if (token == "startpos") {
                board = Board();
            } else if (token == "fen") {
                std::string fen, tmp;
                fen.clear();
                for (int i = 0; i < 6; i++) {
                    iss >> tmp;
                    fen += tmp + " ";
                }
                board.setFen(fen);
            }

            // Apply moves if present
            if (iss >> token && token == "moves") {
                std::string moveStr;
                while (iss >> moveStr) {
                    Move m = uci::uciToMove(board, moveStr);
                    board.makeMove(m);
                }
            }
        }

        // --- Go: play FIRST legal move ---
        else if (line.starts_with("go")) {
            Movelist moves;
            movegen::legalmoves(moves, board);
            int max_player_flag = 0;
            if(board.sideToMove() == Color::WHITE)
            {
                max_player_flag = 1;
            }
            // std::this_thread::sleep_for(std::chrono::seconds(1));
            if (moves.empty()) {
                std::cout << "bestmove 0000\n";
            } else {
                // Move m = moves[0];
                Move m;
                // minimax(board, 0, 7, max_player_flag, -INF, INF, m);
                int score = 0;

                for(int depth = 1; depth < 7; depth++)
                {
                    Move curr_best;
                    score = minimax(board, 0, depth, max_player_flag, -INF, INF, curr_best);
                    m = curr_best;
                }
                std::cout << "bestmove " << uci::moveToUci(m) << "\n";
            }
        }

        // --- Quit ---
        else if (line == "quit") {
            break;
        }
    }

    return 0;
}

int minimax(Board& board, int depth, int max_depth, int max_player_flag, int alpha, int beta, Move&final_move)
{
    auto key = board.hash();
    TTEntry& entry = tt[key%TT_SIZE];
    int remaining = max_depth - depth;
    Move ttmove = Move::NO_MOVE;
    if(entry.hash == key && entry.remaining >= remaining)
    {
        if(entry.flag == 0)
        {
            return entry.score;
        }
        if(entry.flag == 1)
        {
            alpha = std::max(alpha, entry.score);
        }
        if(entry.flag == 2)
        {
            beta = std::min(beta, entry.score);
        }
        if(alpha>=beta)
        {
            return entry.score;
        }

    }

    Movelist moves;
    // pieceValue()
    movegen::legalmoves(moves, board);

    if(depth>=max_depth)
    {
        int score = utility(board, moves);
        tt[key % TT_SIZE] = {key, 0, score, Move::NO_MOVE, 0};
        // if(max_player_flag==0)
        // {
        //     x*=-1;
        // }
        return score;
    }
    
    // std::string fen = board.getFen();
    // auto gameres = board.isGameOver();
    if(moves.empty())
    {
        if(board.inCheck())
        {
            if(board.sideToMove() == Color::WHITE)
            {
                return -MATE+depth;
            }
            else
            {
                return MATE-depth;
            }
        }
        else
        {
            return 0;
        }
    }

    if(entry.hash == key && entry.best_move != Move::NO_MOVE)
    {
        ttmove = entry.best_move;
    }
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b){
    return moveScore(board, a, ttmove) > moveScore(board, b, ttmove);
});

    int bestu = -INF; int worstu = INF;
    Move bestmove = moves[0];
    int orig_alpha = alpha;

    for (const auto &move : moves) {
        board.makeMove(move);
        int nxt = minimax(board, depth+1, max_depth, (max_player_flag+1)%2, alpha, beta, final_move);
        board.unmakeMove(move);
        if(max_player_flag)
        {
            if(nxt>bestu)
            {
                alpha = std::max(alpha, nxt);
                bestmove = move;
                bestu = nxt;
            }
        }
        else
        {
            if(nxt<worstu)
            {
                beta = std::min(beta, nxt);
                bestmove = move;
                worstu = nxt;
            }
        }
        if(alpha>=beta)
        {
            break;
        }
    }

    // std::cout << "Scanned " << fen << "\n";
    if(depth==0)
    {
        final_move = bestmove;
    }

    int score = 0;
    if(max_player_flag)
    {
        // move_map[fen] = bestmove;
        score =  bestu;
    }
    else
    {
        // move_map[fen] = bestmove;
        score = worstu;
    }
    int flag = 0;
    if(score <= orig_alpha)
    {
        flag = 2;
    }
    else if(score >= beta)
    {
        flag = 1;
    }
    tt[key % TT_SIZE] = {key, max_depth-depth, score, bestmove, flag};
    return score;
}

// int quiescence()

int utility(Board& board, Movelist& moves)  
{
    int captures = 0;
    int threats = 0;
    int checks = 0;
    int mobility = 0;
    int opp_mobility = 0;
    for(Move &move:moves)
    {
        mobility++;
        if(board.isCapture(move))
        {
            captures+=PIECE_VALUES[(int)board.at(move.to()).type()];
        }
        board.makeMove(move);
        if(board.inCheck())
        {
            checks++;
        }
        board.unmakeMove(move);
    }
    board.makeNullMove();
    Movelist new_moves;
    movegen::legalmoves(new_moves, board);
    for(Move &move:new_moves)
    {
        opp_mobility++;
        if(board.isCapture(move))
        {
            threats+=PIECE_VALUES[(int)board.at(move.to()).type()];
        }
        // board.makeMove(move);
        // if(board.inCheck())
        // {
        //     checks++;
        // }
        // board.unmakeMove(move);
    }
    board.unmakeNullMove();

    // return 10*checks+5*captures-3*threats;

    int side = (board.sideToMove() == Color::WHITE) ? 1 : -1;

    int score = 0;

    score += 50*checks;
    score += (captures - threats);
    score += (mobility - opp_mobility);
    score *= side;

    score += (board.pieces(PieceType::PAWN, Color::WHITE).count() - board.pieces(PieceType::PAWN,   Color::BLACK).count()) * PAWN_VAL;

    score += (board.pieces(PieceType::KNIGHT, Color::WHITE).count() - board.pieces(PieceType::KNIGHT, Color::BLACK).count()) * KNIGHT_VAL;

    score += (board.pieces(PieceType::BISHOP, Color::WHITE).count() - board.pieces(PieceType::BISHOP, Color::BLACK).count()) * BISHOP_VAL;

    score += (board.pieces(PieceType::ROOK,   Color::WHITE).count() - board.pieces(PieceType::ROOK,   Color::BLACK).count()) * ROOK_VAL;

    score += (board.pieces(PieceType::QUEEN,  Color::WHITE).count() - board.pieces(PieceType::QUEEN,  Color::BLACK).count()) * QUEEN_VAL;

    return score;

}

int material(Board& board)
{
    int score = 0;

    score += (board.pieces(PieceType::PAWN, Color::WHITE).count() - board.pieces(PieceType::PAWN,   Color::BLACK).count()) * PAWN_VAL;

    score += (board.pieces(PieceType::KNIGHT, Color::WHITE).count() - board.pieces(PieceType::KNIGHT, Color::BLACK).count()) * KNIGHT_VAL;

    score += (board.pieces(PieceType::BISHOP, Color::WHITE).count() - board.pieces(PieceType::BISHOP, Color::BLACK).count()) * BISHOP_VAL;

    score += (board.pieces(PieceType::ROOK,   Color::WHITE).count() - board.pieces(PieceType::ROOK,   Color::BLACK).count()) * ROOK_VAL;

    score += (board.pieces(PieceType::QUEEN,  Color::WHITE).count() - board.pieces(PieceType::QUEEN,  Color::BLACK).count()) * QUEEN_VAL;

    return score;
}

int moveScore(Board& board, const Move& move, const Move& ttMove)
{
    if(move == ttMove) return 1000000;      // TT move first
    if(board.isCapture(move))
    {
        int victim   = PIECE_VALUES[board.at(move.to()).type()];
        int attacker = PIECE_VALUES[board.at(move.from()).type()];
        return 100000 + victim - attacker;
    }
    return 0;
}