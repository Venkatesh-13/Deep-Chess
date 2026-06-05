import pygame
import math
import argparse
import copy

# ── argument parsing ──────────────────────────────────────────────────────────
parser = argparse.ArgumentParser()
parser.add_argument('--BotPlayer', type=int, required=True,
                    help='1 or 2  (which player the bot controls)')
parser.add_argument('--NumBoards', type=int, default=2,
                    help='Number of Notakto boards (default 2)')
arguments = parser.parse_args()

NUM_BOARDS = arguments.NumBoards
BOT_PLAYER = arguments.BotPlayer          # 1 or 2

# ── layout constants ──────────────────────────────────────────────────────────
CELL   = 80          # px per cell
BOARD_PX = CELL * 3  # 240 px per board
GAP    = 40          # gap between boards
MARGIN = 40          # left/top margin
TOP_BAR = 50         # height of status bar at top

WIN_W  = MARGIN * 2 + NUM_BOARDS * BOARD_PX + (NUM_BOARDS - 1) * GAP
WIN_H  = TOP_BAR + MARGIN + BOARD_PX + MARGIN

# colours
BG       = (255, 255, 255)
BOARD_C  = (40,  40,  40)
DEAD_BG  = (230, 230, 230)   # greyed-out board background
X_COLOR  = (200, 50,  50)
DEAD_X   = (180, 180, 180)   # x colour on a dead board
HOVER_C  = (200, 230, 255)
TEXT_C   = (20,  20,  20)
P1_COLOR = (200, 50,  50)
P2_COLOR = (50,  100, 200)


# ── History class (minimal, matches q2.py) ────────────────────────────────────
class History:
    def __init__(self, num_boards=2, history=None):
        self.num_boards = num_boards
        if history is not None:
            self.history = history
            self.boards  = self.get_boards()
        else:
            self.history = []
            self.boards  = [['0'] * 9 for _ in range(num_boards)]
        self.active_board_stats = self.check_active_boards()
        self.current_player = self.get_current_player()

    def get_boards(self):
        boards = [['0'] * 9 for _ in range(self.num_boards)]
        for act in self.history:
            board_num      = act // 9
            play_position  = act %  9
            boards[board_num][play_position] = 'x'
        return boards

    @staticmethod
    def is_board_win(board):
        for i in range(3):
            if board[3*i] == board[3*i+1] == board[3*i+2] != '0':
                return True
            if board[i] == board[i+3] == board[i+6] != '0':
                return True
        if board[0] == board[4] == board[8] != '0':
            return True
        if board[2] == board[4] == board[6] != '0':
            return True
        return False

    def check_active_boards(self):
        return [0 if self.is_board_win(b) else 1 for b in self.boards]

    def get_current_player(self):
        return 1 if len(self.history) % 2 == 0 else 2

    def is_terminal(self):
        return all(s == 0 for s in self.active_board_stats)

    def get_valid_actions(self):
        valid = []
        for i, board in enumerate(self.boards):
            if self.active_board_stats[i] == 1:
                for j in range(9):
                    if board[j] == '0':
                        valid.append(9 * i + j)
        return valid

    def apply_action(self, action):
        h = copy.deepcopy(self)
        h.history.append(action)
        h.boards = h.get_boards()
        h.active_board_stats = h.check_active_boards()
        h.current_player = h.get_current_player()
        return h


# ── Bot strategy: alpha-beta on the fly ───────────────────────────────────────
_cache = {}

def _ab(history_obj, alpha, beta):
    """Returns (value, best_action).  Player 1 = max, Player 2 = min."""
    key = (tuple(tuple(b) for b in history_obj.boards),
           history_obj.current_player)
    if key in _cache:
        return _cache[key], None

    if history_obj.is_terminal():
        # the player who just moved completed the last board → they lose
        # current_player is the one who would move next, so the *previous* player lost
        val = 1 if history_obj.current_player == 1 else -1
        _cache[key] = val
        return val, None

    acts   = history_obj.get_valid_actions()
    # move ordering: centre → corners → edges
    def order(a):
        pos = a % 9
        if pos == 4: return 0
        if pos in (0, 2, 6, 8): return 1
        return 2
    acts.sort(key=order)

    best_act = acts[0]
    if history_obj.current_player == 1:
        best_val = -math.inf
        for a in acts:
            val, _ = _ab(history_obj.apply_action(a), alpha, beta)
            if val > best_val:
                best_val, best_act = val, a
            alpha = max(alpha, val)
            if alpha >= beta:
                break
    else:
        best_val = math.inf
        for a in acts:
            val, _ = _ab(history_obj.apply_action(a), alpha, beta)
            if val < best_val:
                best_val, best_act = val, a
            beta = min(beta, val)
            if alpha >= beta:
                break

    _cache[key] = best_val
    return best_val, best_act


def bot_move(history_obj):
    _, action = _ab(history_obj, -math.inf, math.inf)
    return action


# ── drawing helpers ────────────────────────────────────────────────────────────
def board_origin(board_idx):
    """Top-left pixel of board `board_idx`."""
    x = MARGIN + board_idx * (BOARD_PX + GAP)
    y = TOP_BAR + MARGIN
    return x, y


def draw_boards(screen, history_obj, hover_cell=None):
    screen.fill(BG)

    for bi in range(NUM_BOARDS):
        ox, oy = board_origin(bi)
        dead   = history_obj.active_board_stats[bi] == 0

        # board background
        if dead:
            pygame.draw.rect(screen, DEAD_BG, (ox, oy, BOARD_PX, BOARD_PX))

        # grid lines
        line_c = BOARD_C
        for i in range(4):
            pygame.draw.line(screen, line_c, (ox + i*CELL, oy),
                             (ox + i*CELL, oy + BOARD_PX), 2)
            pygame.draw.line(screen, line_c, (ox, oy + i*CELL),
                             (ox + BOARD_PX, oy + i*CELL), 2)

        # hover highlight
        if hover_cell is not None:
            hb, hpos = hover_cell
            if hb == bi and not dead:
                hx = ox + (hpos % 3) * CELL + 2
                hy = oy + (hpos // 3) * CELL + 2
                pygame.draw.rect(screen, HOVER_C, (hx, hy, CELL-4, CELL-4))

        # x marks
        board = history_obj.boards[bi]
        xc = DEAD_X if dead else X_COLOR
        for pos in range(9):
            if board[pos] == 'x':
                cx = ox + (pos % 3) * CELL
                cy = oy + (pos // 3) * CELL
                pad = 18
                pygame.draw.line(screen, xc,
                                 (cx+pad, cy+pad), (cx+CELL-pad, cy+CELL-pad), 7)
                pygame.draw.line(screen, xc,
                                 (cx+pad, cy+CELL-pad), (cx+CELL-pad, cy+pad), 7)

        # "dead" label
        if dead:
            font_sm = pygame.font.SysFont('arialunicode', 20, bold=True)
            lbl = font_sm.render('dead', True, (160, 160, 160))
            lx  = ox + BOARD_PX // 2 - lbl.get_width() // 2
            ly  = oy + BOARD_PX // 2 - lbl.get_height() // 2
            screen.blit(lbl, (lx, ly))


def draw_status(screen, history_obj, game_over, winner, font):
    if game_over:
        pc = P1_COLOR if winner == 1 else P2_COLOR
        msg = f'Player {winner} wins!   (press Y to restart, N to quit)'
    else:
        cp  = history_obj.current_player
        pc  = P1_COLOR if cp == 1 else P2_COLOR
        tag = ' (bot)' if cp == BOT_PLAYER else ' (you)'
        msg = f"Player {cp}'s turn{tag}"
    lbl = font.render(msg, True, pc)
    screen.blit(lbl, (MARGIN, 12))


def cell_at(mx, my):
    """Return (board_idx, cell_pos) under mouse, or None."""
    for bi in range(NUM_BOARDS):
        ox, oy = board_origin(bi)
        if ox <= mx < ox + BOARD_PX and oy <= my < oy + BOARD_PX:
            col = (mx - ox) // CELL
            row = (my - oy) // CELL
            return bi, row * 3 + col
    return None


# ── main ──────────────────────────────────────────────────────────────────────
def main():
    pygame.init()
    screen = pygame.display.set_mode((WIN_W, WIN_H))
    pygame.display.set_caption(f'Notakto  ({NUM_BOARDS} board{"s" if NUM_BOARDS>1 else ""})')

    font     = pygame.font.SysFont('arialunicode', 24)
    clock    = pygame.time.Clock()
    click_delay   = 300
    last_click    = 0

    history_obj   = History(num_boards=NUM_BOARDS)
    game_over     = False
    winner        = None
    hover_cell    = None

    # if bot goes first, compute its move immediately
    waiting_for_bot = (history_obj.current_player == BOT_PLAYER)

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_y and game_over:
                    history_obj   = History(num_boards=NUM_BOARDS)
                    game_over     = False
                    winner        = None
                    hover_cell    = None
                    waiting_for_bot = (history_obj.current_player == BOT_PLAYER)
                elif event.key == pygame.K_n:
                    running = False

            if not game_over and not waiting_for_bot:
                if event.type == pygame.MOUSEMOTION:
                    res = cell_at(*event.pos)
                    if res is not None:
                        bi, pos = res
                        action = 9 * bi + pos
                        if action in history_obj.get_valid_actions():
                            hover_cell = (bi, pos)
                        else:
                            hover_cell = None
                    else:
                        hover_cell = None

                if event.type == pygame.MOUSEBUTTONDOWN:
                    now = pygame.time.get_ticks()
                    if now - last_click > click_delay:
                        last_click = now
                        res = cell_at(*event.pos)
                        if res is not None:
                            bi, pos = res
                            action  = 9 * bi + pos
                            if action in history_obj.get_valid_actions():
                                history_obj = history_obj.apply_action(action)
                                hover_cell  = None
                                if history_obj.is_terminal():
                                    # last to move loses in Notakto:
                                    # current_player is the one who would move next
                                    # the player who just moved completed every board → they lose
                                    loser  = 2 if history_obj.current_player == 1 else 1
                                    winner = 2 if loser == 1 else 1
                                    game_over = True
                                else:
                                    waiting_for_bot = (history_obj.current_player == BOT_PLAYER)

        # bot move
        if not game_over and waiting_for_bot:
            pygame.time.wait(150)                    # tiny pause so it feels natural
            action = bot_move(history_obj)
            if action is not None:
                history_obj = history_obj.apply_action(action)
                if history_obj.is_terminal():
                    loser  = 2 if history_obj.current_player == 1 else 1
                    winner = 2 if loser == 1 else 1
                    game_over = True
                waiting_for_bot = False
            else:
                # no valid action → terminal
                game_over = True
                winner = history_obj.current_player  # opponent wins

        draw_boards(screen, history_obj, hover_cell)
        draw_status(screen, history_obj, game_over, winner, font)

        if game_over:
            font_sm = pygame.font.SysFont('arialunicode', 18)
            hint = font_sm.render('press Y to play again  |  N to quit', True, (120, 120, 120))
            screen.blit(hint, (MARGIN, WIN_H - 28))

        pygame.display.flip()
        clock.tick(60)

    pygame.quit()


if __name__ == '__main__':
    main()