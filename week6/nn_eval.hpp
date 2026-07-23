// nn_eval.hpp
//
// Drop-in NNUE-style eval for Venky_Engine.
// Include this after "chess.hpp". Requires weights_export/{w1,b1,w2,b2,w3,b3}.bin
// (produced by export_weights.py) to be readable at engine startup.

#pragma once
#include "chess.hpp"
#include <fstream>
#include <vector>
#include <array>
#include <cmath>
#include <stdexcept>

namespace nneval {

using namespace chess;

inline const PieceType PIECE_ORDER[6] = {
    PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
    PieceType::ROOK, PieceType::QUEEN,  PieceType::KING
};

inline int piece_index_of(PieceType pt) {
    for (int i = 0; i < 6; i++) if (PIECE_ORDER[i] == pt) return i;
    return -1;
}

inline int feature_index(int sq, PieceType pt, Color color) {
    int pi = piece_index_of(pt);
    int color_offset = (color == Color::WHITE) ? 0 : 6;
    return sq * 12 + pi + color_offset;
}

inline constexpr int NUM_FEATURES = 769;     // 768 piece features + 1 side-to-move
inline constexpr int STM_FEATURE_INDEX = 768;
inline constexpr int HIDDEN1_SIZE = 512;     // <-- change this to resize hidden layer 1
inline constexpr int HIDDEN2_SIZE = 64;      // <-- change this to resize hidden layer 2

inline std::array<float, NUM_FEATURES> board_to_vector(const Board& board) {
    std::array<float, NUM_FEATURES> vec{};
    vec.fill(0.0f);
    for (int sq = 0; sq < 64; sq++) {
        Piece piece = board.at(Square(sq));
        if (piece == Piece::NONE) continue;
        vec[feature_index(sq, piece.type(), piece.color())] = 1.0f;
    }
    vec[STM_FEATURE_INDEX] = (board.sideToMove() == Color::WHITE) ? 1.0f : 0.0f;
    return vec;
}

struct FeatureToggle { int idx; int sign; };

// Fixed-capacity diff: at most 4 piece-feature changes (castling) + 1 side-to-move
// toggle (which flips on EVERY move) = 5 max. No heap allocation.
struct MoveDiff {
    std::array<FeatureToggle, 5> toggles;
    int count = 0;
    void add(FeatureToggle t) { toggles[count++] = t; }
};

// Must be called BEFORE board.makeMove(move) -- reads pre-move board state.
inline MoveDiff compute_move_diff(const Board& board, const Move& move) {
    MoveDiff diff;

    Square from = move.from();
    Square to = move.to();
    Piece moving_piece = board.at(from);
    PieceType moving_type = moving_piece.type();
    Color side = moving_piece.color();

    diff.add({feature_index(from.index(), moving_type, side), -1});

    // Side to move ALWAYS flips after a move. If White is moving, the STM
    // feature goes 1 -> 0 (remove); if Black is moving, it goes 0 -> 1 (add).
    if (side == Color::WHITE) diff.add({STM_FEATURE_INDEX, -1});
    else                      diff.add({STM_FEATURE_INDEX, +1});

    if (move.typeOf() == Move::CASTLING) {
        bool kingside = to.index() > from.index();
        int rank = from.index() / 8;
        Square king_to = kingside ? Square(rank * 8 + 6) : Square(rank * 8 + 2);
        Square rook_from = to;
        Square rook_to = kingside ? Square(rank * 8 + 5) : Square(rank * 8 + 3);

        diff.add({feature_index(king_to.index(), PieceType::KING, side), +1});
        diff.add({feature_index(rook_from.index(), PieceType::ROOK, side), -1});
        diff.add({feature_index(rook_to.index(), PieceType::ROOK, side), +1});
        return diff;
    }

    if (move.typeOf() == Move::ENPASSANT) {
        Square captured_sq = Square(static_cast<int>(to.index()) + (side == Color::WHITE ? -8 : 8));
        Piece captured = board.at(captured_sq);
        diff.add({feature_index(captured_sq.index(), captured.type(), captured.color()), -1});
    } else if (board.at(to) != Piece::NONE) {
        Piece captured = board.at(to);
        diff.add({feature_index(to.index(), captured.type(), captured.color()), -1});
    }

    if (move.typeOf() == Move::PROMOTION) {
        diff.add({feature_index(to.index(), move.promotionType(), side), +1});
    } else {
        diff.add({feature_index(to.index(), moving_type, side), +1});
    }

    return diff;
}

inline std::vector<float> load_bin(const std::string& path, size_t count) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("nn_eval: could not open " + path);
    std::vector<float> data(count);
    f.read(reinterpret_cast<char*>(data.data()), count * sizeof(float));
    if (!f) throw std::runtime_error("nn_eval: short read on " + path);
    return data;
}

class ChessEvalNet {
public:
    std::vector<float> w1, b1, w2, b2, w3, b3;

    void load(const std::string& dir) {
        w1 = load_bin(dir + "/w1.bin", HIDDEN1_SIZE * NUM_FEATURES);
        b1 = load_bin(dir + "/b1.bin", HIDDEN1_SIZE);
        w2 = load_bin(dir + "/w2.bin", HIDDEN2_SIZE * HIDDEN1_SIZE);
        b2 = load_bin(dir + "/b2.bin", HIDDEN2_SIZE);
        w3 = load_bin(dir + "/w3.bin", 1 * HIDDEN2_SIZE);
        b3 = load_bin(dir + "/b3.bin", 1);
    }

    static float relu(float x) { return x > 0.0f ? x : 0.0f; }
    static float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }

    std::array<float, HIDDEN1_SIZE> init_accumulator(const std::array<float, NUM_FEATURES>& x) const {
        std::array<float, HIDDEN1_SIZE> acc{};
        for (int o = 0; o < HIDDEN1_SIZE; o++) {
            float sum = b1[o];
            const float* wrow = &w1[o * NUM_FEATURES];
            for (int i = 0; i < NUM_FEATURES; i++) sum += x[i] * wrow[i];
            acc[o] = sum;
        }
        return acc;
    }

    inline void add_feature(std::array<float, HIDDEN1_SIZE>& acc, int idx) const {
        for (int o = 0; o < HIDDEN1_SIZE; o++) acc[o] += w1[o * NUM_FEATURES + idx];
    }
    inline void remove_feature(std::array<float, HIDDEN1_SIZE>& acc, int idx) const {
        for (int o = 0; o < HIDDEN1_SIZE; o++) acc[o] -= w1[o * NUM_FEATURES + idx];
    }

    float forward_from_accumulator(const std::array<float, HIDDEN1_SIZE>& accumulator) const {
        std::array<float, HIDDEN1_SIZE> h1{};
        for (int o = 0; o < HIDDEN1_SIZE; o++) h1[o] = relu(accumulator[o]);

        std::array<float, HIDDEN2_SIZE> h2{};
        for (int o = 0; o < HIDDEN2_SIZE; o++) {
            float sum = b2[o];
            const float* wrow = &w2[o * HIDDEN1_SIZE];
            for (int i = 0; i < HIDDEN1_SIZE; i++) sum += h1[i] * wrow[i];
            h2[o] = relu(sum);
        }
        float sum = b3[0];
        for (int i = 0; i < HIDDEN2_SIZE; i++) sum += h2[i] * w3[i];
        return sigmoid(sum);
    }
};

inline float winprob_to_cp(float p) {
    p = std::max(1e-6f, std::min(1.0f - 1e-6f, p));
    return -400.0f * std::log(1.0f / p - 1.0f);
}

// Apply a precomputed diff to the accumulator (direction = +1 forward / -1 to undo)
inline void apply_diff(const ChessEvalNet& net, std::array<float, HIDDEN1_SIZE>& acc,
                        const MoveDiff& diff, int direction) {
    for (int i = 0; i < diff.count; i++) {
        int effective_sign = diff.toggles[i].sign * direction;
        if (effective_sign > 0) net.add_feature(acc, diff.toggles[i].idx);
        else net.remove_feature(acc, diff.toggles[i].idx);
    }
}

} // namespace nneval