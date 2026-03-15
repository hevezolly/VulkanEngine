#pragma once

#include <render_node.h>
#include <tuple>
#include <utility>
#include <allocator_feature.h>
#include <helper_functions.h>

template<typename... Bindings>
struct RenderNodeWithBindings: RenderNode {

    using RenderNode::RenderNode;

    void SetBindings(Bindings&... b) {
        _bindings = std::tuple<Bindings...>(std::forward<Bindings>(b)...);
    }

    virtual uint32_t getInputDependenciesCount() {
        return (Bindings::size_inputs() + ... + 0);
    }

    virtual uint32_t getOutputDependenciesCount() {
        return (Bindings::size_outputs() + ... + 0);
    }

    virtual void getInputDependencies(NodeDependency* dependencies) {
        WriteInputsEach(dependencies, 0, std::index_sequence_for<Bindings...>{});
    }

    virtual void getOutputDependencies(NodeDependency* dependencies) {
        WriteOutputsEach(dependencies, Attachments::size(), std::index_sequence_for<Bindings...>{});
    }

private:

    template<size_t... Is>
    void WriteInputsEach(NodeDependency* dependencies, uint32_t offset, std::index_sequence<Is...>) {
        (std::get<Is>(_bindings).write_inputs(dependencies + offset + Is), ...);
    }

    template<size_t... Is>
    void WriteOutputsEach(NodeDependency* dependencies, uint32_t offset, std::index_sequence<Is...>) {
        (std::get<Is>(_bindings).write_outputs(dependencies + offset + Is), ...);
    }

    template<size_t... Is>
    void FillShaderInstances(ShaderInputInstance* buffer, Descriptors& descriptors, std::index_sequence<Is...>) {
        (new (buffer + Is) ShaderInputInstance(descriptors.BorrowDescriptorSet(std::get<Is>(_bindings))), ...);
    }

protected:

    MemChunk<ShaderInputInstance> getShaderInputs() {
        MemChunk<ShaderInputInstance> inputBuffer = Helpers::allocator(&context)
            .BumpAllocate<ShaderInputInstance>(sizeof...(Bindings));

        Descriptors& descriptors = Helpers::getDescriptors(&context);

        FillShaderInstances(inputBuffer.data, descriptors, std::index_sequence_for<Bindings...>{});

        return inputBuffer;
    }

    std::tuple<Bindings...> _bindings;
};