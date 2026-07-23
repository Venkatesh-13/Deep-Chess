"""
Anchor our engine's strength against Stockfish limited to a target Elo
(UCI_LimitStrength + UCI_Elo). Gives an ABSOLUTE rating estimate.

USAGE:
  python anchor.py --engine "./engine_v3.exe weights_base200.txt" --elo 1800 --games 20 --movetime 0.1
"""
import argparse, math
import chess, chess.engine

SF = "stockfish"

OPENINGS = [
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
    "rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 2",
    "rnbqkb1r/pppppppp/5n2/8/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 2",
    "rnbqkbnr/pppp1ppp/8/4p3/2P5/8/PP1PPPPP/RNBQKBNR w KQkq - 0 2",
    "rnbqkbnr/pp1ppppp/8/2p5/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 2",
    "rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
]


def play(board, w, b, mt):
    while not board.is_game_over(claim_draw=True):
        eng = w if board.turn == chess.WHITE else b
        r = eng.play(board, chess.engine.Limit(time=mt))
        if r.move is None: break
        board.push(r.move)
    return board.result(claim_draw=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", required=True)
    ap.add_argument("--elo", type=int, default=1800)
    ap.add_argument("--games", type=int, default=20)
    ap.add_argument("--movetime", type=float, default=0.1)
    a = ap.parse_args()

    me = chess.engine.SimpleEngine.popen_uci(a.engine.split())
    sf = chess.engine.SimpleEngine.popen_uci([SF])
    sf.configure({"UCI_LimitStrength": True, "UCI_Elo": a.elo})

    w = l = d = 0
    try:
        for g in range(a.games):
            board = chess.Board(OPENINGS[(g // 2) % len(OPENINGS)])
            me_white = (g % 2 == 0)
            r = play(board, me, sf, a.movetime) if me_white else play(board, sf, me, a.movetime)
            if (r == "1-0" and me_white) or (r == "0-1" and not me_white): w += 1
            elif r == "1/2-1/2": d += 1
            else: l += 1
            print(f"game {g+1}/{a.games} me({'W' if me_white else 'B'}) {r}  W-L-D {w}-{l}-{d}", flush=True)
    finally:
        me.quit(); sf.quit()

    score = (w + 0.5 * d) / max(1, w + l + d)
    print(f"\nvs Stockfish@{a.elo}: W-L-D {w}-{l}-{d}  score {score:.3f}")
    if 0 < score < 1:
        gap = -400 * math.log10(1 / score - 1)
        print(f"  => our Elo ~= {a.elo + gap:.0f}  (gap {gap:+.0f} vs SF@{a.elo})")
    else:
        print(f"  => our Elo {'>>' if score==1 else '<<'} {a.elo}")


if __name__ == "__main__":
    main()