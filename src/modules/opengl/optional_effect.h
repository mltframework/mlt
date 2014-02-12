#ifndef OPTIONAL_EFFECT_H
#define OPTIONAL_EFFECT_H

#include <movit/effect.h>
#include <movit/effect_chain.h>
#include <assert.h>

// A wrapper effect that, at rewrite time, can remove itself entirely from the loop.
// It does so if "disable" is set to a nonzero value at finalization time.
template<class T>
class OptionalEffect : public T {
public:
	OptionalEffect() : disable(0) { this->register_int("disable", &disable); }
	virtual std::string effect_type_id() const { return "OptionalEffect[" + T::effect_type_id() + "]"; }
	virtual void rewrite_graph(movit::EffectChain *graph, movit::Node *self) {
		if (disable) {
			assert(self->incoming_links.size() == 1);
			graph->replace_sender(self, self->incoming_links[0]);
			self->disabled = true;
		} else {
			T::rewrite_graph(graph, self);
		}
	}

private:
	int disable;
};

#endif
