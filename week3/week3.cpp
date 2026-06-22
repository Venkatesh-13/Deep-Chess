#include "chess.hpp"

using namespace chess;

int minimax(Board& board, int depth, std::unordered_map<std::string, Move>&move_map, int max_player_flag, int alpha, int beta);

int main () {
    // Board board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Movelist moves;
    // movegen::legalmoves(moves, board);

    // for (const auto &move : moves) {
    //     std::cout << uci::moveToUci(move) << std::endl;
    // }

    // return 0;
    std::string fen; std::cin >> fen;

    
    Board board(fen);
    // Movelist moves;
    // movegen::legalmoves(moves, board);

    // for (const auto &move : moves) {
    //     std::cout << uci::moveToUci(move) << std::endl;
    // }
    std::unordered_map<std::string, Move> move_map;
    int max_player_flag = 0;
    if(board.sideToMove() == Color::WHITE)
    {
        max_player_flag = 1;
    }
    minimax(board, 0, move_map, max_player_flag, -100, 100) << '\n';
    Movelist moves;
    movegen::legalmoves(moves, board);
    while(true)
    {
        movegen::legalmoves(moves, board);
        if((moves.empty() && board.inCheck()))
        {
            break;
        }
        std::string fen = board.getFen();
        auto move = move_map[fen];
        std::cout << uci::moveToSan(board, move) << std::endl;
        board.makeMove(move);
    }
}

int minimax(Board& board, int depth, std::unordered_map<std::string, Move>&move_map, int max_player_flag, int alpha, int beta)
{
    if(depth>7)
    {
        return 0;
    }
    Movelist moves;
    movegen::legalmoves(moves, board);
    std::string fen = board.getFen();
    // auto gameres = board.isGameOver();
    if(moves.empty())
    {
        if(board.inCheck())
        {
            if(board.sideToMove() == Color::WHITE)
            {
                return -10;
            }
            else
            {
                return 10;
            }
        }
        else
        {
            return 0;
        }
    }

    int bestu = -100; int worstu = 100;
    Move bestmove;
    auto it = move_map.find(fen);
    if(it!=move_map.end())
    {
        Move move = move_map[fen];
        board.makeMove(move);
        int nxt = minimax(board, depth+1, move_map, (max_player_flag+1)%2, alpha, beta);
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
    }
    else
    {
    for (const auto &move : moves) {
        board.makeMove(move);
        int nxt = minimax(board, depth+1, move_map, (max_player_flag+1)%2, alpha, beta);
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
    }}

    // std::cout << "Scanned " << fen << "\n";
    if(max_player_flag)
    {
        move_map[fen] = bestmove;
        return bestu;
    }
    else
    {
        move_map[fen] = bestmove;
        return worstu;
    }   
}