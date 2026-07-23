import argparse
import numpy as np
import pandas as pd
import chess
import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader, random_split

PIECE_TYPES = [chess.PAWN, chess.KNIGHT, chess.BISHOP, chess.ROOK, chess.QUEEN, chess.KING]

def fen_to_vector(fen: str) -> np.ndarray:
    board = chess.Board(fen)
    vec = np.zeros(769, dtype=np.float32)
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece is None:
            continue
        piece_index = PIECE_TYPES.index(piece.piece_type)
        color_offset = 0 if piece.color == chess.WHITE else 6
        vec[square * 12 + piece_index + color_offset] = 1.0
    vec[768] = 1.0 if board.turn == chess.WHITE else 0.0
    return vec

def parse_eval(value) -> float:
    s = str(value).strip()
    if s.startswith('#'):
        mate_in = int(s[1:].replace('+', ''))
        sign = 1 if mate_in > 0 else -1
        return sign * (10000 - abs(mate_in) * 10)
    return float(s)

class ChessEvalDataset(Dataset):
    def __init__(self, fens: np.ndarray, scores_cp: np.ndarray):
        self.fens = fens
        self.scores_cp = scores_cp.astype(np.float32)

    def __len__(self):
        return len(self.fens)

    def __getitem__(self, idx):
        x = fen_to_vector(self.fens[idx])
        # squash centipawn score to a bounded ~win-probability target
        y = 1.0 / (1.0 + np.exp(-self.scores_cp[idx] / 400.0))
        return torch.from_numpy(x), torch.tensor(y, dtype=torch.float32)

class ChessEvalNet(nn.Module):
    def __init__(self):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(769, 512),
            nn.ReLU(),
            nn.Linear(512, 64),
            nn.ReLU(),
            nn.Linear(64, 1),
            nn.Sigmoid(),
        )

    def forward(self, x):
        return self.net(x).squeeze(-1)   # shape (batch,) to match targets

def train(csv_path: str, epochs: int, batch_size: int, lr: float, val_fraction: float, out_path: str):
    print(f"Loading {csv_path} ...")
    df = pd.read_csv(csv_path)
    df['cp'] = df['Evaluation'].apply(parse_eval)
    print(f"Loaded {len(df):,} rows")

    full_dataset = ChessEvalDataset(df['FEN'].values, df['cp'].values)

    val_size = int(len(full_dataset) * val_fraction)
    train_size = len(full_dataset) - val_size
    train_ds, val_ds = random_split(full_dataset, [train_size, val_size])
    print(f"Train: {train_size:,}  Val: {val_size:,}")

    train_loader = DataLoader(train_ds, batch_size=batch_size, shuffle=True, num_workers=0)
    val_loader = DataLoader(val_ds, batch_size=batch_size, shuffle=False, num_workers=0)

    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f"Using device: {device}")

    model = ChessEvalNet().to(device)
    loss_fn = nn.MSELoss()
    optimizer = torch.optim.Adam(model.parameters(), lr=lr)

    for epoch in range(1, epochs + 1):
        model.train()
        train_loss_sum = 0.0
        for xb, yb in train_loader:
            xb, yb = xb.to(device), yb.to(device)

            pred = model(xb)
            loss = loss_fn(pred, yb)

            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

            train_loss_sum += loss.item() * len(xb)
        train_loss = train_loss_sum / train_size

        model.eval()
        val_loss_sum = 0.0
        with torch.no_grad():
            for xb, yb in val_loader:
                xb, yb = xb.to(device), yb.to(device)
                pred = model(xb)
                loss = loss_fn(pred, yb)
                val_loss_sum += loss.item() * len(xb)
        val_loss = val_loss_sum / val_size if val_size > 0 else float('nan')

        print(f"epoch {epoch:3d}/{epochs}  train_loss={train_loss:.5f}  val_loss={val_loss:.5f}")

    torch.save(model.state_dict(), out_path)
    print(f"Saved model weights to {out_path}")
    return model

def evaluate_fen(model: ChessEvalNet, fen: str, device='cpu') -> float:
    model.eval()
    x = torch.from_numpy(fen_to_vector(fen)).unsqueeze(0).to(device)
    with torch.no_grad():
        return model(x).item()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--csv', type=str, required=True, help='Path to FEN,Evaluation CSV')
    parser.add_argument('--epochs', type=int, default=20)
    parser.add_argument('--batch_size', type=int, default=1024)
    parser.add_argument('--lr', type=float, default=1e-3)
    parser.add_argument('--val_fraction', type=float, default=0.05)
    parser.add_argument('--out', type=str, default='chess_eval_model.pt')
    args = parser.parse_args()

    model = train(args.csv, args.epochs, args.batch_size, args.lr, args.val_fraction, args.out)

    # quick sanity check on the starting position
    start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    print(f"\nStarting position win-prob estimate: {evaluate_fen(model, start_fen):.4f} (should be near 0.5)")