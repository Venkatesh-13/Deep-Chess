"""
Export a trained ChessEvalNet's weights to plain binary files
that C++ can load with no PyTorch/Python dependency.

Usage:
    python3 export_weights.py --model chess_eval_model.pt --out_dir weights_export
"""
import argparse
import os
import numpy as np
import torch

from train_chess_eval import ChessEvalNet


def export(model_path: str, out_dir: str):
    os.makedirs(out_dir, exist_ok=True)

    model = ChessEvalNet()
    model.load_state_dict(torch.load(model_path, map_location='cpu'))
    model.eval()

    # net.0 = Linear(768,256), net.2 = Linear(256,32), net.4 = Linear(32,1)
    # (indices 1,3,5 are ReLU/Sigmoid - no weights)
    layers = {
        'w1': model.net[0].weight.detach().numpy(),  # shape (256, 768)
        'b1': model.net[0].bias.detach().numpy(),     # shape (256,)
        'w2': model.net[2].weight.detach().numpy(),   # shape (32, 256)
        'b2': model.net[2].bias.detach().numpy(),     # shape (32,)
        'w3': model.net[4].weight.detach().numpy(),   # shape (1, 32)
        'b3': model.net[4].bias.detach().numpy(),     # shape (1,)
    }

    shapes_lines = []
    for name, arr in layers.items():
        arr = arr.astype(np.float32)
        path = os.path.join(out_dir, f'{name}.bin')
        arr.tofile(path)
        shapes_lines.append(f"{name} {' '.join(str(d) for d in arr.shape)}")
        print(f"{name}: shape={arr.shape}  ->  {path}  ({arr.nbytes} bytes)")

    with open(os.path.join(out_dir, 'shapes.txt'), 'w') as f:
        f.write('\n'.join(shapes_lines) + '\n')

    print(f"\nDone. Exported to {out_dir}/")
    print("Files: w1.bin, b1.bin, w2.bin, b2.bin, w3.bin, b3.bin, shapes.txt")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--model', type=str, default='chess_eval_model.pt')
    parser.add_argument('--out_dir', type=str, default='weights_export')
    args = parser.parse_args()
    export(args.model, args.out_dir)