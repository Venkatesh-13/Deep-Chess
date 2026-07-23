#include <bits/stdc++.h>
#include "chess.hpp"

using namespace chess;

#define MATE 100000
#define INF 1000000

const int QUEEN_VAL = 900;
const int ROOK_VAL = 500;
const int BISHOP_VAL = 300;
const int KNIGHT_VAL = 300;
const int PAWN_VAL = 100;

int node_count = 0;

const int PST_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
const int PST_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};
const int PST_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};
const int PST_ROOK[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};
const int PST_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
const int PST_KING[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

const int* PST[6] = {
    PST_PAWN, PST_KNIGHT, PST_BISHOP,
    PST_ROOK, PST_QUEEN,  PST_KING
};



auto search_start = std::chrono::steady_clock::now();
int time_limit_ms = 1000;
bool time_up = false;

int utility(Board& board, Movelist& moves);
int minimax(Board& board, int depth, int max_depth, int max_player_flag, int alpha, int beta, Move&final_move);
int moveScore(Board& board, const Move& move, const Move& ttMove);
int material(Board& board);
int quiescence(Board& board, int alpha, int beta, int max_player_flag, int qdepth);
int pst_score(Board& board, PieceType pt, Color color);
int king_safety(Board& board, Color color);

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
            std::memset(tt, 0, sizeof(tt));
            node_count = 0;
            int my_time = 30000; // default 30 seconds
            int increment = 0;
            std::istringstream iss(line);
            std::string token;
            while(iss >> token)
            {
                if(token == "wtime" && board.sideToMove() == Color::WHITE) iss >> my_time;
                if(token == "btime" && board.sideToMove() == Color::BLACK) iss >> my_time;
                if(token == "winc"  && board.sideToMove() == Color::WHITE) iss >> increment;
                if(token == "binc"  && board.sideToMove() == Color::BLACK) iss >> increment;
            }

            // use 1/20th of remaining time + half increment
            time_limit_ms = (my_time / 20) + (increment / 2);
            time_limit_ms = std::max(50, time_limit_ms); // minimum 50ms
            
            search_start = std::chrono::steady_clock::now();
            time_up = false;

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
                Move m = moves[0];
                // minimax(board, 0, 7, max_player_flag, -INF, INF, m);
                int score = 0;

                for(int depth = 1; depth < 9; depth++)
                {
                    Move curr_best;
                    score = minimax(board, 0, depth, max_player_flag, -INF, INF, curr_best);
                    if(!time_up) m = curr_best;
                    if(time_up) break;
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
    if(++node_count >= 1000)
    {
        node_count = 0;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - search_start).count();
        if(elapsed >= time_limit_ms) time_up = true;
    }
    if(time_up) return 0;
    if(board.isRepetition(2)) return 0;
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

    if(depth>=max_depth)
    {
        // int score = utility(board, moves);
        int score = quiescence(board, alpha, beta, max_player_flag, 0);
        tt[key % TT_SIZE] = {key, 0, score, Move::NO_MOVE, 0};
        // if(max_player_flag==0)
        // {
        //     x*=-1;
        // }
        return score;
    }

    Movelist moves;
    // pieceValue()
    movegen::legalmoves(moves, board);
    
    // std::string fen = board.getFen();
    // auto gameres = board.isGameOver();
    if(moves.empty())
    {
        if(board.inCheck())
        {
            if(board.sideToMove() == Color::WHITE)
            {
                return -MATE-(60-depth)*1000;
            }
            else
            {
                return MATE+(60-depth)*1000;
            }
        }
        else
        {
            return 0;
        }
    }

    if(board.isHalfMoveDraw()) return 0;

    if(entry.hash == key && entry.best_move != Move::NO_MOVE)
    {
        ttmove = Move::NO_MOVE;
        bool found = false;
        for(const auto& m : moves)
        {
            if(m == entry.best_move) 
            {
                found = true; break;
            }
        }
        if(found) ttmove = entry.best_move;
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
        // if(board.isCapture(move))
        // {
        //     captures+=PIECE_VALUES[(int)board.at(move.to()).type()];
        // }
        // board.makeMove(move);
        // if(board.inCheck())
        // {
        //     checks++;
        // }
        // board.unmakeMove(move);
    }
    if(!board.inCheck()){
    board.makeNullMove();
    Movelist new_moves;
    movegen::legalmoves(new_moves, board);
    for(Move &move:new_moves)
    {
        opp_mobility++;
        // if(board.isCapture(move))
        // {
        //     threats+=PIECE_VALUES[(int)board.at(move.to()).type()];
        // }
    }
    board.unmakeNullMove();}

    // return 10*checks+5*captures-3*threats;

    int side = (board.sideToMove() == Color::WHITE) ? 1 : -1;

    int score = 0;

    score += 25*checks;
    score += (captures - threats)*2;
    score += (mobility - opp_mobility) * 3;
    score *= side;

    score += king_safety(board, Color::WHITE);
    score -= king_safety(board, Color::BLACK);

    for(int i = 0; i < 6; i++)
    {
        PieceType pt = PieceType(static_cast<PieceType::underlying>(i));
        score += pst_score(board, pt, Color::WHITE);
        score -= pst_score(board, pt, Color::BLACK);
    }

    score += (board.pieces(PieceType::PAWN, Color::WHITE).count() - board.pieces(PieceType::PAWN,   Color::BLACK).count()) * PAWN_VAL;

    score += (board.pieces(PieceType::KNIGHT, Color::WHITE).count() - board.pieces(PieceType::KNIGHT, Color::BLACK).count()) * KNIGHT_VAL;

    score += (board.pieces(PieceType::BISHOP, Color::WHITE).count() - board.pieces(PieceType::BISHOP, Color::BLACK).count()) * BISHOP_VAL;

    score += (board.pieces(PieceType::ROOK,   Color::WHITE).count() - board.pieces(PieceType::ROOK,   Color::BLACK).count()) * ROOK_VAL;

    score += (board.pieces(PieceType::QUEEN,  Color::WHITE).count() - board.pieces(PieceType::QUEEN,  Color::BLACK).count()) * QUEEN_VAL;

    return score;
}

int quiescence(Board& board, int alpha, int beta, int max_player_flag, int qdepth)
{
    Movelist moves;
    movegen::legalmoves(moves, board);

    if(moves.empty())
    {
        if(board.inCheck())
        {
            // Mated - return from white's perspective
            return (board.sideToMove() == Color::WHITE) ? -MATE : MATE;
        }
        return 0; // stalemate
    }

    int stand_pat = utility(board, moves); // just material, no tactics

    if(qdepth > 8)
    {
        return stand_pat;
    }

    if(max_player_flag)
    {
        if(stand_pat >= beta) return beta;
        alpha = std::max(alpha, stand_pat);
    }
    else
    {
        if(stand_pat <= alpha) return alpha;
        beta = std::min(beta, stand_pat);
    }

    for(const auto& move : moves)
    {
        bool is_capture = board.isCapture(move);
        bool in_check = board.inCheck();
        
        board.makeMove(move);
        bool gives_check = board.inCheck();  // check AFTER making
        
        if(!is_capture && !in_check && !gives_check)
        {
            board.unmakeMove(move);
            continue;  // skip quiet non-checking moves
        }
        
        int score = quiescence(board, alpha, beta, (max_player_flag+1)%2, qdepth+1);
        board.unmakeMove(move);
        if(max_player_flag)
        {
            alpha = std::max(alpha, score);
            if(alpha >= beta) return beta;
        }
        else
        {
            beta = std::min(beta, score);
            if(alpha >= beta) return alpha;
        }
        // ... rest
    }
    return max_player_flag ? alpha : beta;
}

int material(Board& board)
{
    int score = 0;

    score += (board.pieces(PieceType::PAWN, Color::WHITE).count() - board.pieces(PieceType::PAWN,   Color::BLACK).count()) * PAWN_VAL;

    score += (board.pieces(PieceType::KNIGHT, Color::WHITE).count() - board.pieces(PieceType::KNIGHT, Color::BLACK).count()) * KNIGHT_VAL;

    score += (board.pieces(PieceType::BISHOP, Color::WHITE).count() - board.pieces(PieceType::BISHOP, Color::BLACK).count()) * BISHOP_VAL;

    score += (board.pieces(PieceType::ROOK,   Color::WHITE).count() - board.pieces(PieceType::ROOK,   Color::BLACK).count()) * ROOK_VAL;

    score += (board.pieces(PieceType::QUEEN,  Color::WHITE).count() - board.pieces(PieceType::QUEEN,  Color::BLACK).count()) * QUEEN_VAL;
    
    // for(int i = 0; i < 6; i++)
    // {
    //     PieceType pt = PieceType(static_cast<PieceType::underlying>(i));
    //     score += pst_score(board, pt, Color::WHITE);
    //     score -= pst_score(board, pt, Color::BLACK);
    // }

    return score;
}

int moveScore(Board& board, const Move& move, const Move& ttMove)
{
    if(move == ttMove && move != Move::NO_MOVE) return 1000000;      // TT move first
    if(board.isCapture(move))
    {
        int victim   = PIECE_VALUES[board.at(move.to()).type()];
        int attacker = PIECE_VALUES[board.at(move.from()).type()];
        return 100000 + victim - attacker;
    }
    return 0;
}

int pst_score(Board& board, PieceType pt, Color color)
{
    int score = 0;
    Bitboard bb = board.pieces(pt, color);
    while(!bb.empty())
    {
        int sq = bb.lsb();
        int idx = (color == Color::WHITE) ? (63 - sq) : sq;
        score += PST[(int)pt][idx];
        bb.pop();
    }
    return score;
}

int king_safety(Board& board, Color color)
{
    Square king = board.kingSq(color);
    int kf = (int)king.file();
    int kr = (int)king.rank();
    int score = 0;

    Bitboard pawns = board.pieces(PieceType::PAWN, color);
    while(!pawns.empty())
    {
        int sq = pawns.lsb();
        int pf = sq % 8;
        int pr = sq / 8;
        int file_dist = std::abs(pf - kf);
        int rank_dist = (color == Color::WHITE) ? (pr - kr) : (kr - pr);
        if(file_dist <= 1 && rank_dist >= 1 && rank_dist <= 2)
            score += 15;
        pawns.pop();
    }

    for(int f = std::max(0, kf-1); f <= std::min(7, kf+1); f++)
    {
        bool friendly_pawn = false;
        Bitboard fp = board.pieces(PieceType::PAWN, color);
        while(!fp.empty())
        {
            int sq = fp.lsb();
            if(sq % 8 == f) { friendly_pawn = true; break; }
            fp.pop();
        }
        if(!friendly_pawn) score -= 10;
    }

    return score;
}

std::string get_epd(Board& board)
{
    std::string fen = board.getFen();
    fen = fen.substr(0, fen.rfind(' ')); // remove fullmove
    fen = fen.substr(0, fen.rfind(' ')); // remove halfmove
    return fen;
}