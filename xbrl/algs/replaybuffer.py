import numpy as np
from collections import deque
from typing import Tuple


class SimpleBuffer:

    def __init__(self, capacity: int) -> None:
        self.buffer = deque(maxlen=capacity)

    def __len__(self) -> None:
        return len(self.buffer)

    def append(self, experience: Tuple) -> None:
        """Add experience to the buffer.
        """
        self.buffer.append(experience)

    def get_all(self):
        return self.sample(size=len(self.buffer))

    def sample(self, size:int):
        indices = np.random.choice(len(self.buffer), min(size, len(self.buffer)), replace=False)
        out = (np.array(el) for el in zip(*(self.buffer[idx] for idx in indices)))
        return out