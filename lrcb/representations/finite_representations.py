#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Oct 29 10:41:52 2020

@author: papini
"""
import numpy as np
from sklearn.decomposition import PCA
from sklearn.preprocessing import normalize as normalize_matrix


"""Finite linear representation"""
class LinearRepresentation:
    def __init__(self, features, param):
        self.features = features
        self.n_contexts, self.n_arms, self.dim = features.shape
        assert self.dim == len(param)
        self._param = param
        
        self._rewards = np.matmul(self.features, self._param)
        self._optimal_arms = np.argmax(self._rewards, axis=1)
        ii = np.arange(self.n_contexts)
        self._optimal_rewards = self._rewards[ii, self._optimal_arms]
        self._optimal_features = self.features[ii, self._optimal_arms, :]
        
    def __eq__(self, other):
        return np.allclose(self._rewards, other._rewards)
    

#Diversity properties    
def rank(rep, tol=None):
    all_feats = np.reshape(rep.features, 
                           (rep.n_contexts * rep.n_arms, rep.dim))
    return np.linalg.matrix_rank(all_feats, tol)

def spans(rep, tol=None):
    return rank(rep, tol) == rep.dim


def is_cmb(rep, tol=None):
    for a in range(rep.n_arms):
        feats = rep.features[:, a, :]
        if np.linalg.matrix_rank(feats, tol) < rep.dim:
            return False
    return True

def hls_rank(rep, tol=None):
    return np.linalg.matrix_rank(rep._optimal_features, tol)

def is_hls(rep, tol=None):
    return hls_rank(rep, tol) == rep.dim


#Making representations
def make_canon_rep(n_contexts, n_arms, normalize=True):
    dim = n_contexts * n_arms
    features = np.eye(dim)
    features = np.reshape(features, (n_contexts, n_arms, dim))
    param = 2 * np.random.uniform(size=dim) - 1
    
    r1 = LinearRepresentation(features, param)
    
    if normalize:
        r1 = normalize_param(r1)
    
    return r1

def make_random_rep(n_contexts, n_arms, dim, ortho=True, normalize=True):
    features = np.random.normal(size=(n_contexts, n_arms, dim))
    param = 2 * np.random.uniform(size=dim) - 1
    
    #Orthogonalize features
    if ortho:
        features = np.reshape(features, (n_contexts * n_arms, dim))
        orthogonalizer = PCA(n_components=dim) #no dimensionality reduction
        features = orthogonalizer.fit_transform(features)
        features = np.reshape(features, (n_contexts, n_arms, dim))
        features = np.take(features, np.random.permutation(dim), axis=2)
    
    r1 = LinearRepresentation(features, param)
    
    if normalize:
        r1 = normalize_param(r1)
        
    return r1

#Transforming representations
def normalize_param(rep, scale=1.):
    param = rep._param
    param_norm = np.linalg.norm(param)
    param = param / param_norm * scale
    
    features = rep.features * param_norm / scale
    
    return LinearRepresentation(features, param)

def random_transform(rep, normalize=True):
    dim = rep.dim
    A = np.random.normal(size=(dim, dim))
    orthogonalizer = PCA(n_components=dim)
    A = orthogonalizer.fit_transform(A)
    A = normalize_matrix(A, axis=0, norm='l2')
    features = np.matmul(rep.features, A)
    param = np.matmul(A.T, rep._param)
    
    r1 = LinearRepresentation(features, param)
    
    if normalize:
        r1 = normalize_param(r1)
        
    assert r1 == rep
    return r1 

def derank_hls(rep, newrank=1, transform=True, normalize=True):
    f0 = rep.features
    opt_feats = rep._optimal_features
    opt_arms = rep._optimal_arms
    nc = rep.n_contexts
    opt_rews = rep._optimal_rewards.reshape((nc, 1)) 
    remove = min(max(nc - newrank + 1, 0), nc)
    
    f1 = np.array(f0)
    outer = np.matmul(opt_rews[:remove], opt_rews[:remove].T)
    xx = np.matmul(outer, opt_feats[:remove, :]) \
        / np.linalg.norm(opt_rews[:remove])**2
    f1[np.arange(remove), opt_arms[:remove], :] = xx
    
    param = np.array(rep._param)
    
    r1 = LinearRepresentation(f1, param)
    
    if transform:
        r1 = random_transform(r1, normalize=normalize_param)
    elif normalize:
        r1 = normalize_param(r1)
        
    assert r1 == rep
    return r1

"""
def make_hls_rank(rewards, dim, rank, transform=True, eps=0.1):
    opt_rewards = np.max(rewards, axis=1)
    assert np.linalg.norm(opt_rewards) > 0
    nc, na = rewards.shape 
    
    param = np.zeros(dim)
    param[0] = 1
    sup = np.max(opt_rewards) + eps
    features = np.zeros((nc, na, dim))
    features[:, :, 0] = rewards
    for i in range(1, rank):
        features[i, :, i] = sup
    
    if transform:
        dim = rep.dim
        A = np.random.normal(size=(dim, dim))
        orthogonalizer = PCA(n_components=dim)
        A = orthogonalizer.fit_transform(A)
        A = normalize(A, axis=0, norm='l2')
        f1 = np.matmul(f1, A)
        param = np.matmul(A.T, param)
        if normalize_param:
            param_norm = np.linalg.norm(param)
            param = param / param_norm
            f1 = f1 * param_norm
    
    return LinearRepresentation(features, param)
"""

#Examples
_param = np.ones(2)
"""Example 1: HLS and CMB"""
EX1 = LinearRepresentation(features=np.array([[[2., 0.], [1., 0.]], 
                                              [[0., 2.], [0., 1.]]], 
                                             dtype=np.float64),
                            param=_param)

"""Example 2: HLS, not CMB"""
EX2 = LinearRepresentation(features=np.array([[[2., 0.], [0.5, 0.5]], 
                                              [[0., 2.], [0.5, 0.5]]], 
                                             dtype=np.float64),
                            param=_param)

"""Example 3: CMB, not HLS"""
EX3 = LinearRepresentation(features=np.array([[[1., 1.], [1., 0.]], 
                                              [[0., 1.], [1., 1.]]], 
                                             dtype=np.float64),
                            param=_param)

"""Example 4: not CMB nor HLS"""
EX4 = LinearRepresentation(features=np.array([[[1., 1.], [0.5, 0.5]], 
                                              [[0., 1.], [1., 1.]]], 
                                             dtype=np.float64),
                            param=_param)

#Sanity checks
if __name__ == '__main__':
    #Test properties
    assert is_hls(EX1)
    assert is_cmb(EX1)
    
    assert is_hls(EX2)
    assert not is_cmb(EX2)
    
    assert not is_hls(EX3)
    assert is_cmb(EX3)
    
    assert not is_hls(EX4)
    assert not is_cmb(EX4)
    