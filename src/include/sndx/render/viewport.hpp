#pragma once

#include "../collision/rect.hpp"

#include "../math/math.hpp"

#include <glm/glm.hpp>

namespace sndx::render {

	template <typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
	class Viewport {
	public:
		using Vec = glm::vec<2, InternalT, Qualifier>;
		using RectT = sndx::collision::Rect<Vec>;

	protected:
		RectT m_rect{};

	public:

		explicit Viewport(Vec dims, Vec offset = Vec(InternalT(0.0))):
			m_rect(offset, offset + dims) {
			
			if (glm::compMin(dims) <= InternalT(0.0))
				throw std::invalid_argument("Dimensions of Viewport must be > 0");
		}

		virtual ~Viewport() noexcept = default;

		[[nodiscard]]
		constexpr operator const RectT& () const noexcept {
			return m_rect;
		}

		[[nodiscard]]
		constexpr virtual double getAspectRatio() const noexcept {
			auto dims = m_rect.getSize();

			return double(dims.x) / double(dims.y);
		}

		[[nodiscard]]
		constexpr auto getDimensions() const noexcept {
			return m_rect.getSize();
		}

		[[nodiscard]]
		constexpr auto getOffset() const noexcept {
			return m_rect.getPosition();
		}

		constexpr virtual void setOffset(Vec offset) noexcept {
			m_rect.setPosDims(offset, m_rect.getSize());
		}

		virtual void resize(Vec newDims) {
			if (glm::compMin(newDims) <= InternalT(0.0))
				throw std::invalid_argument("Dimensions of Viewport must be > 0");

			m_rect.setPosDims(m_rect.getPosition(), newDims);
		}

		[[nodiscard]]
		constexpr Vec pixToNDC(Vec in) const noexcept {
			return sndx::math::remap(in, m_rect.getP1(), m_rect.getP2(), Vec(-1.0), Vec(1.0));
		}

		[[nodiscard]]
		constexpr Vec NDCtoPix(const Vec& ndc) const noexcept {
			return sndx::math::remap(ndc, Vec(-1.0), Vec(1.0), m_rect.getP1(), m_rect.getP2());
		}
	};

	template <typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
	class AspectRatioViewport : public Viewport<InternalT, Qualifier> {
	public:
		using Vec = Viewport<InternalT, Qualifier>::Vec;
		using RectT = Viewport<InternalT, Qualifier>::RectT;

	protected:
		Vec m_alignment = Vec(InternalT(0.5));
		InternalT m_aspectRatio;

	public:
		explicit AspectRatioViewport(Vec dims, InternalT aspectRatio, Vec alignment = Vec(InternalT(0.5))):
			Viewport<InternalT, Qualifier>(dims), m_alignment{}, m_aspectRatio(1.0) {

			setAspectRatio(aspectRatio);
			setAlignment(alignment);
			resize(dims);
		}

		[[nodiscard]]
		constexpr double getAspectRatio() const noexcept override  {
			return m_aspectRatio;
		}

		double setAspectRatio(InternalT aspectRatio) {
			if (aspectRatio <= InternalT(0.0))
				throw std::invalid_argument("Aspect Ratio must be > 0.0");

			return std::exchange(m_aspectRatio, aspectRatio);
		}

		double setAspectRatio(InternalT aspectRatio, Vec dims) {
			auto tmp = setAspectRatio(aspectRatio);
			resize(dims);

			return tmp;
		}

		Vec setAlignment(Vec alignment) {
			if (glm::compMin(alignment) < InternalT(0.0))
				throw std::invalid_argument("Alignment must be above or equal to 0.0");

			if (glm::compMax(alignment) > InternalT(1.0))
				throw std::invalid_argument("Alignment must be below or equal to 1.0");

			return std::exchange(m_alignment, alignment);
		}

		Vec setAlignment(Vec alignment, Vec dims) {
			auto tmp = setAlignment(alignment);
			resize(dims);

			return tmp;
		}

		virtual void resize(Vec newDims) override {
			if (glm::compMin(newDims) <= InternalT(0.0))
				throw std::invalid_argument("Dimensions of Viewport must be > 0");

			auto asWidth = std::max(newDims.y * m_aspectRatio, InternalT(1.0));

			if (asWidth == newDims.x) {
				this->m_rect.setPosDims(Vec(InternalT(0.0)), newDims);
				return;
			}

			auto asHeight = std::max(newDims.x / m_aspectRatio, InternalT(1.0));

			if (newDims.x > asWidth) {
				auto padding = (newDims.x - asWidth) * m_alignment.x;
				this->m_rect.setPosDims(Vec(padding, InternalT(0.0)), Vec(asWidth, newDims.y));
				return;
			}

			auto padding = (newDims.y - asHeight) * m_alignment.y;
			this->m_rect.setPosDims(Vec(InternalT(0.0), padding), Vec(newDims.x, asHeight));
		}
	};
}