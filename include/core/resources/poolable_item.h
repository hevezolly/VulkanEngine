#pragma once

struct PoolableItem {
    virtual void OnPool();
    virtual void OnReturn();
};