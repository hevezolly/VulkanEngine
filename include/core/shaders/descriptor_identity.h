#pragma once
#include <common.h>
#include <resource_id.h>
#include <hash_combine.h>

struct DescriptorIdentity {

    ResourceId resource;
    union Additional
    {
        struct BufferRange{
            uint32_t offset;
            uint32_t size;
        } bufferRange;
        ResourceId sampler;
    } additional;

    bool operator==(const DescriptorIdentity& other) const {
        ResourceType type = resource.type();
        return resource == other.resource &&
            ((type == ResourceType::Buffer && 
                additional.bufferRange.offset == other.additional.bufferRange.offset &&
                additional.bufferRange.size == other.additional.bufferRange.size) ||
             (type == ResourceType::Image && 
                additional.sampler == other.additional.sampler) ||
             (type == ResourceType::Sampler || type == ResourceType::Other)
            );
    }
    
    bool operator!=(const DescriptorIdentity& other) const {
        return !(operator==(other));
    }
};

namespace std {
    template<>
    struct hash<DescriptorIdentity> {
        size_t operator()(const DescriptorIdentity& i) const noexcept {
            size_t seed = 0;
            hash_combine(seed, i.resource);
            if (i.resource.type() == ResourceType::Buffer) {
                hash_combine(seed, i.additional.bufferRange.offset);
                hash_combine(seed, i.additional.bufferRange.size);
            } else if (i.resource.type() == ResourceType::Image && 
                       i.additional.sampler.type() == ResourceType::Sampler)
            {
                hash_combine(seed, i.additional.sampler);
            }

            return seed;
        }
    };
}

struct DescriptorSetIdentity {

    std::vector<DescriptorIdentity> identities;
    std::size_t hash = 0;

    bool operator==(const DescriptorSetIdentity& other) const {
        if (hash != other.hash)
            return false;

        if (identities.size() != other.identities.size())
            return false;

        for (int i = 0; i < identities.size(); i++) {
            if (identities[i] != other.identities[i])
                return false;
        }

        return true;
    }

    DescriptorSetIdentity(DescriptorSetIdentity&&) = default;
    DescriptorSetIdentity& operator=(DescriptorSetIdentity&&) = default;
    DescriptorSetIdentity(const DescriptorSetIdentity&) = default;
    DescriptorSetIdentity& operator=(const DescriptorSetIdentity&) = default;

    
    bool operator!=(const DescriptorSetIdentity& other) const {
        return !(operator==(other));
    }

    void clear() {
        identities.clear();
        hash = 0;
    }

    void add(const DescriptorIdentity& id) {
        hash_combine(hash, id);
        identities.push_back(id);
    }
};

namespace std {
    template<>
    struct hash<DescriptorSetIdentity> {
        size_t operator()(const DescriptorSetIdentity& i) const noexcept {
            return i.hash;
        }
    };
}