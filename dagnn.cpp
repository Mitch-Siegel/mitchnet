#include "dagnn.h"
#include "queue"
#include "stack"

namespace SimpleNets
{
    DAGNetwork::DAGNetwork(size_t nInputs,
                           std::vector<std::pair<neuronTypes, size_t>> hiddenNeurons,
                           std::pair<size_t, neuronTypes> outputFormat)
    {
        this->layers.push_back(Layer(this, true));
        size_t i;
        for (i = 0; i < nInputs; i++)
        {
            this->layers.back().AddUnit(this->GenerateUnitFromType(input));
        }

        Layer hiddenLayer = Layer(this, false);
        for (i = 0; i < hiddenNeurons.size(); i++)
        {
            hiddenLayer.AddUnit(this->GenerateUnitFromType(hiddenNeurons[i].first, hiddenNeurons[i].second));
        }

        // for(i = 0; i < hiddenNeurons.size(); i++)
        // {
        // }

        this->layers.push_back(hiddenLayer);
        /*for (i = 0; i < hiddenLayers.size(); i++)
        {
            Layer *l = new Layer(this, false);
            for (size_t j = 0; j < hiddenLayers[i].first; j++)
            {
                Unit *u = this->GenerateUnitFromType(hiddenLayers[i].second);
                for (auto k = this->layers.back()->begin(); k != this->layers.back()->end(); ++k)
                {
                    u->AddConnection(*k, 0.1);
                }
                l->AddUnit(u);
            }
            this->layers.push_back(l);
        }*/

        Layer ol = Layer(this, false);
        for (size_t j = 0; j < outputFormat.first; j++)
        {
            ol.AddUnit(this->GenerateUnitFromType(outputFormat.second));
        }
        this->layers.push_back(ol);
    }

    DAGNetwork::~DAGNetwork()
    {
    }

    void DAGNetwork::Recalculate()
    {
        std::set<Unit *> touched;
        std::queue<Unit *> toRecalculate;
        for (auto u = this->layers.back().begin(); u != this->layers.back().end(); ++u)
        {
            toRecalculate.push((*u));
        }
        while (toRecalculate.size() > 0)
        {
            Unit *u = toRecalculate.front();
            if (touched.count(u) == 0)
            {
                // if we have reached an input, nothing to do
                if (u->InboundConnections().size() == 0)
                {
                    toRecalculate.pop();
                }
                // otherwise, push all this node's inputs in front of it in the queue
                else
                {
                    for (auto c : u->InboundConnections())
                    {
                        toRecalculate.push(c->from);
                    }
                }
            }
            else
            {
                u->CalculateValue();
                toRecalculate.pop();
            }
            touched.insert(u);
        }
    }

    void DAGNetwork::GeneratePostNumbers()
    {
        size_t post = 0;
        this->postNumbers.clear();
        std::stack<Unit *> exploreStack;
        Layer &ol = this->layers.back();
        for (auto u = ol.begin(); u != ol.end(); ++u)
        {
            exploreStack.push(*u);
        }
        std::set<Unit *> visited;

        while (exploreStack.size() > 0)
        {
            Unit *j = exploreStack.top();
            if (visited.count(j) == 0)
            {
                for (auto c : j->InboundConnections())
                {
                    Unit *i = c->from;
                    exploreStack.push(i);
                }
            }

            if (visited.count(j))
            {
                exploreStack.pop();
                postNumbers[post] = j;
                post++;
            }
            else
            {
                visited.insert(j);
            }
        }
    }

    /*
    bool DAGNetwork::CheckForCycle()
    {
        std::stack<Unit *> exploreStack;
        // std::map<Unit *, bool> visited;
        // std::map<
        std::set<Unit *> visited;
        std::set<Unit *> parent;
        // std::vector<bool> visited(this->units().size(), false);
        // std::vector<bool> parent(this->units().size(), false);

        while (exploreStack.size() > 0)
        {
            Unit *j = exploreStack.top();
            visited.insert(j);
            bool needPop = true;

            for (auto c : j->InboundConnections())
            {
                Unit *i = c->from;
                if (visited.count(i) && parent.count(i))
                {
                    return true;
                }
                else if (!needPop)
                {
                    exploreStack.push(i);
                    needPop = false;
                }
            }

            if (needPop)
            {
                parent.erase(j);
                exploreStack.pop();
            }
        }

        return false;
    }
    */
    bool DAGNetwork::OnConnectionAdded(Connection *c)
    {
        std::stack<Unit *> exploreStack;
        std::set<Unit *> visited;
        exploreStack.push(c->from);
        bool seenStartingUnit = false;
        while (exploreStack.size() > 0)
        {
            Unit *u = exploreStack.top();
            // if (visited.count(u))
            if (u == c->from)
            {
                if (seenStartingUnit)
                {
                    this->RemoveConnection(c);
                    return true;
                }
                else
                {
                    seenStartingUnit = true;
                }
            }
            exploreStack.pop();
            for (auto c : u->OutboundConnections())
            {
                exploreStack.push(c->to);
            }
        }

        this->GeneratePostNumbers();
        return false;
    }

    bool DAGNetwork::OnConnectionRemoved(Connection *c)
    {
        this->GeneratePostNumbers();
        return false;
    }

    void DAGNetwork::Learn(const std::vector<nn_num_t> &expectedOutput, nn_num_t learningRate)
    {
        if (expectedOutput.size() != this->layers.back().size())
        {
            printf("Provided expected output array of length %lu, expected size %lu\n",
                   expectedOutput.size(), this->layers.back().size());
            exit(1);
        }
        this->BackPropagate(expectedOutput);
        this->UpdateWeights(learningRate);
    }

    nn_num_t DAGNetwork::Output()
    {
        this->Recalculate();
        switch (this->layers.back().size())
        {
        case 1:
            return (this->layers.back()[0].Activation());
            break;

        default:
            int maxIndex = 0;
            nn_num_t maxValue = -1.0 * MAXFLOAT;
            Layer &ol = this->layers.back();
            for (size_t i = 0; i < ol.size(); i++)
            {
                nn_num_t thisOutput = ol[i].Activation();
                if (thisOutput > maxValue)
                {
                    maxValue = thisOutput;
                    maxIndex = i;
                }
            }
            return (nn_num_t)maxIndex;
        }
        return -999.999;
    }

    void DAGNetwork::BackPropagate(const std::vector<nn_num_t> &expectedOutput)
    {

        for (auto u : this->units())
        {
            u.second->delta = 0.0;
        }
        // delta of each output j = activation derivative(j) * (expected(j) - actual(j))
        std::set<Unit *> visited;
        std::queue<Unit *> toPropagate;
        Layer &ol = this->layers.back();
        for (size_t j = 0; j < ol.size(); j++)
        {
            ol[j].delta = ol[j].ActivationDeriv() * (expectedOutput[j] - ol[j].Activation());
            toPropagate.push(&ol[j]);
        }
        while (toPropagate.size() > 0)
        {
            Unit *j = toPropagate.front();
            toPropagate.pop();
            if (visited.count(j) == 0)
            {
                visited.insert(j);
                for (auto c : j->InboundConnections())
                {
                    Unit *i = c->from;
                    i->delta += (c->weight * j->delta);
                    toPropagate.push(i);
                }
            }
        }

        for (auto u : this->units())
        {
            u.second->delta *= u.second->ActivationDeriv();
        }
    }

    void DAGNetwork::UpdateWeights(nn_num_t learningRate)
    {
        for (auto u : this->units())
        {
            Unit *j = u.second;
            for (auto v : j->InboundConnections())
            {
                Unit *i = v->from;
                j->ChangeConnectionWeight(i, nullptr, learningRate * i->Activation() * j->delta);
            }
        }
    }

    void DAGNetwork::PrintPOSTNumbers()
    {
        this->GeneratePostNumbers();
        printf("POST numbers for DAGNetwork:\n");
        for (auto p : this->postNumbers)
        {
            printf("POST %lu: Unit %lu\n", p.first, p.second->Id());
        }
    }
} // namespace SimpleNets