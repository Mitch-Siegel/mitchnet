#include "nnet.h"

namespace SimpleNets
{
    class FeedForwardNeuralNet : public NeuralNet
    {

    private:
        bool finalized;
        void BackPropagate(const std::vector<nn_num_t> &expectedOutput);
        void UpdateWeights(nn_num_t learningRate);
        void ForwardPropagate();

        bool OnConnectionAdded(Connection *c);
        bool OnConnectionRemoved(Connection *c);

    public:
        FeedForwardNeuralNet(size_t nInputs,
                             std::vector<std::pair<size_t, neuronTypes>> hiddenLayers,
                             std::pair<size_t, neuronTypes> outputFormat);
        ~FeedForwardNeuralNet();

        void AddLayer(size_t size, enum neuronTypes t);

        void Learn(const std::vector<nn_num_t> &expectedOutput, nn_num_t learningRate);

        nn_num_t Output() override;
    };
} // namespace SimpleNets
