import numpy as np
from ..contextualfinite import ContextualFinite

"""
Just a ContextualFinite carrying a parameter. Is this to generalize disjoint
and shared-parameter linear contextual bandits?
"""
class LinearCB(ContextualFinite):
    def __init__(self, features, param, rewards, random_state=0) -> None:
        super().__init__(features=features, labels=rewards, random_state=random_state)
        self.param = param

class LinearRandom(LinearCB):

    def __init__(self, n_contexts=20, n_actions=4, feature_dim=10, disjoint=False, random_state=0) -> None:
        self.disjoint = disjoint
        random_state = random_state
        rng = np.random.RandomState(random_state)
        features = rng.randn(n_contexts, n_actions, feature_dim)
        if self.disjoint:
            theta = rng.randn(n_actions, feature_dim) * 2 - 1
            rewards = np.sum(features * theta, axis=-1)
        else:
            theta = rng.randn(feature_dim) * 2 - 1
            rewards = features @ theta
        super().__init__(features=features, 
                         param=theta, 
                         rewards=rewards, 
                         random_state=random_state)