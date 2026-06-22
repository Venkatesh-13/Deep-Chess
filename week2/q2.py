import copy  # use it for deepcopy if needed
import math
import logging

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

# Global variable to keep track of visited board positions. This is a dictionary with keys as self.boards as str and
# value represents the maxmin value. Use the get_boards_str function in History class to get the key corresponding to
# self.boards.
board_positions_val_dict = {}
# Global variable to store the visited histories in the process of alpha beta pruning.
visited_histories_list = []


class History:
    def __init__(self, num_boards=2, history=None):
        """
        # self.history : Eg: [0, 4, 2, 5]
            keeps track of sequence of actions played since the beginning of the game.
            Each action is an integer between 0-(9n-1) representing the square in which the move will be played as shown
            below (n=2 is the number of boards).

             Board 1
              ___ ___ ____
             |_0_|_1_|_2_|
             |_3_|_4_|_5_|
             |_6_|_7_|_8_|

             Board 2
              ____ ____ ____
             |_9_|_10_|_11_|
             |_12_|_13_|_14_|
             |_15_|_16_|_17_|

        # self.boards
            empty squares are represented using '0' and occupied squares are 'x'.
            Eg: [['x', '0', 'x', '0', 'x', 'x', '0', '0', '0'], ['0', 0', '0', 0', '0', 0', '0', 0', '0']]
            for two board game

            Board 1
              ___ ___ ____
             |_x_|___|_x_|
             |___|_x_|_x_|
             |___|___|___|

            Board 2
              ___ ___ ____
             |___|___|___|
             |___|___|___|
             |___|___|___|

        # self.player: 1 or 2
            Player whose turn it is at the current history/board

        :param num_boards: Number of boards in the game of Notakto.
        :param history: list keeps track of sequence of actions played since the beginning of the game.
        """
        self.num_boards = num_boards
        if history is not None:
            self.history = history
            self.boards = self.get_boards()
        else:
            self.history = []
            self.boards = []
            for i in range(self.num_boards):
                # empty boards
                self.boards.append(['0', '0', '0', '0', '0', '0', '0', '0', '0'])
        # Maintain a list to keep track of active boards
        self.active_board_stats = self.check_active_boards()
        self.current_player = self.get_current_player()

    def get_boards(self):
        """ Play out the current self.history and get the boards corresponding to the history.

        :return: list of lists
                Eg: [['x', '0', 'x', '0', 'x', 'x', '0', '0', '0'], ['0', 0', '0', 0', '0', 0', '0', 0', '0']]
                for two board game

                Board 1
                  ___ ___ ____
                 |_x_|___|_x_|
                 |___|_x_|_x_|
                 |___|___|___|

                Board 2
                  ___ ___ ____
                 |___|___|___|
                 |___|___|___|
                 |___|___|___|
        """
        boards = []
        for i in range(self.num_boards):
            boards.append(['0', '0', '0', '0', '0', '0', '0', '0', '0'])
        for i in range(len(self.history)):
            board_num = math.floor(self.history[i] / 9)
            play_position = self.history[i] % 9
            boards[board_num][play_position] = 'x'
        return boards

    def check_active_boards(self):
        """ Return a list to keep track of active boards

        :return: list of int (zeros and ones)
                Eg: [0, 1]
                for two board game

                Board 1
                  ___ ___ ____
                 |_x_|_x_|_x_|
                 |___|_x_|_x_|
                 |___|___|___|

                Board 2
                  ___ ___ ____
                 |___|___|___|
                 |___|___|___|
                 |___|___|___|
        """
        active_board_stat = []
        for i in range(self.num_boards):
            if self.is_board_win(self.boards[i]):
                active_board_stat.append(0)
            else:
                active_board_stat.append(1)
        return active_board_stat

    @staticmethod
    def is_board_win(board):
        for i in range(3):
            if board[3 * i] == board[3 * i + 1] == board[3 * i + 2] != '0':
                return True

            if board[i] == board[i + 3] == board[i + 6] != '0':
                return True

        if board[0] == board[4] == board[8] != '0':
            return True

        if board[2] == board[4] == board[6] != '0':
            return True
        return False

    def get_current_player(self):
        """
        Get player whose turn it is at the current history/board
        :return: 1 or 2
        """
        total_num_moves = len(self.history)
        if total_num_moves % 2 == 0:
            return 1
        else:
            return 2

    def get_boards_str(self):
        boards_str = ""
        for i in range(self.num_boards):
            boards_str = boards_str + ''.join([str(j) for j in self.boards[i]])
        return boards_str

    def is_win(self):
        # Feel free to implement this in anyway if needed
        for board in self.boards:
            if not self.is_board_win(board):
                return False
        return True

    def get_valid_actions(self):
        # Feel free to implement this in anyway if needed
        valid=[]
        valid_boards = self.check_active_boards()
        for i, board in enumerate(self.boards):
            if valid_boards[i]>0:
                for j in range(9):
                    if board[j]=='0':
                        valid.append(9*i+j)
        
        return valid

    def is_terminal_history(self):
        # Feel free to implement this in anyway if needed
        return self.is_win()

    def get_value_given_terminal_history(self):
        # Feel free to implement this in anyway if needed
        curr = self.get_current_player()
        if curr==1:
            return 1
        else:
            return -1
        
    def update_history(self, action):
        self.history.append(action)
        board_num = action//9
        act = action%9
        self.boards[board_num][act] = 'x'
        # self.boards = self.get_boards()

    def undo_last(self):
        action = self.history[-1]
        self.history.pop(-1)
        board_num = action//9
        act = action%9
        self.boards[board_num][act] = '0'
        # self.boards = self.get_boards()


def alpha_beta_pruning(history_obj: History, alpha, beta, max_player_flag):
    """
        Calculate the maxmin value given a History object using alpha beta pruning. Use the specific move order to
        speedup (more pruning, less memory).

    :param history_obj: History class object
    :param alpha: -math.inf
    :param beta: math.inf
    :param max_player_flag: Bool (True if maximizing player plays)
    :return: float
    """
    # These two already given lines track the visited histories.
    global visited_histories_list
    visited_histories_list.append(history_obj.history)
    # TODO implement
    if history_obj.is_terminal_history():
        return history_obj.get_value_given_terminal_history()
    curr = history_obj.get_current_player()
    acts = history_obj.get_valid_actions()
    ordered_acts = []
    for act in acts:
        if act%9 == 4:
            ordered_acts.append(act)
    for act in acts:
        if act%9 in [0,2,6,8]:
            ordered_acts.append(act)
    for act in acts:
        if act%9 in [1,3,5,7]:
            ordered_acts.append(act)
    if curr==1:
        bestu = -67
        for act in ordered_acts:
            # new_hist = copy.deepcopy(history_obj)
            # history_obj.update_history(act)
            history_obj.update_history(act)
            val = alpha_beta_pruning(history_obj, alpha, beta, history_obj.get_current_player() == 1)
            history_obj.undo_last()
            if val>bestu:
                bestu = val
            alpha = max(val, alpha)
            if alpha>=beta:
                break
            
        return bestu
    else:
        worstu = 67
        for act in ordered_acts:
            # new_hist = copy.deepcopy(history_obj)
            # history_obj.update_history(act)
            history_obj.update_history(act)
            val = alpha_beta_pruning(history_obj, alpha, beta, history_obj.get_current_player() == 1)
            history_obj.undo_last()
            if val<worstu:
                worstu = val
            beta = min(val, beta)
            if alpha>=beta:
                break
            
        return worstu
    return -2
    # TODO implement


def maxmin(history_obj: History, max_player_flag):
    """
        Calculate the maxmin value given a History object using maxmin rule. Store the value of already visited
        board positions to speed up, avoiding recursive calls for a different history with the same board position.
    :param history_obj: History class object
    :param max_player_flag: True if the player is maximizing player
    :return: float
    """
    # Global variable to keep track of visited board positions. This is a dictionary with keys as str version of
    # self.boards and value represents the maxmin value. Use the get_boards_str function in History class to get
    # the key corresponding to self.boards.
    global board_positions_val_dict
    # TODO implement
    curr = history_obj.get_current_player()

    if history_obj.is_terminal_history():
        return history_obj.get_value_given_terminal_history()
    if history_obj.get_boards_str() in board_positions_val_dict:
        return board_positions_val_dict[history_obj.get_boards_str()]
    acts = history_obj.get_valid_actions()
    ordered_acts = []
    for act in acts:
        if act%9 == 4:
            ordered_acts.append(act)
    for act in acts:
        if act%9 in [0,2,6,8]:
            ordered_acts.append(act)
    for act in acts:
        if act not in ordered_acts:
            ordered_acts.append(act)
    
    if max_player_flag:
        bestu = -67
        for act in ordered_acts:
            new_hist = copy.deepcopy(history_obj)
            new_hist.update_history(act)
            board_str = new_hist.get_boards_str()
            u = 0
            if board_str in board_positions_val_dict:
                u = board_positions_val_dict[board_str]
            else:
                u = maxmin(new_hist, False)

            if u > bestu:
                bestu = u

        board_positions_val_dict[history_obj.get_boards_str()] = bestu
        return bestu

            
    else:
        worstu = 67
        for act in ordered_acts:
            new_hist = copy.deepcopy(history_obj)
            new_hist.update_history(act)
            board_str = new_hist.get_boards_str()
            u = 0
            if board_str in board_positions_val_dict:
                u = board_positions_val_dict[board_str]
            else:
                u = maxmin(new_hist, True)

            if u < worstu:
                worstu = u

        board_positions_val_dict[history_obj.get_boards_str()] = worstu
        return worstu
    return -2
    # TODO implement


def solve_alpha_beta_pruning(history_obj, alpha, beta, max_player_flag):
    global visited_histories_list
    val = alpha_beta_pruning(history_obj, alpha, beta, max_player_flag)
    return val, visited_histories_list


if __name__ == "__main__":
    logging.info("start")
    logging.info("alpha beta pruning")
    value, visited_histories = solve_alpha_beta_pruning(History(history=[], num_boards=3), -math.inf, math.inf, True)
    logging.info("maxmin value {}".format(value))
    logging.info("Number of histories visited {}".format(len(visited_histories)))
    logging.info("maxmin memory")
    logging.info("maxmin value {}".format(maxmin(History(history=[], num_boards=2), True)))
    logging.info("end")
