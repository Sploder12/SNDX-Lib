#pragma once

#include "../collision/rect.hpp"

#include "../math/math.hpp"

#include <glm/glm.hpp>

namespace sndx::render {

	template <VectorN<2> VecT = glm::vec2>
	class Viewport {
	public:
		using Vec = VecT;
		using RectT = sndx::collision::Rect<VecT>;
		using Precision = typename RectT::Precision;

	protected:
		RectT m_rect{};

	public:

		explicit Viewport(Vec dims, Vec offset = Vec(Precision(0.0))):
			m_rect(offset, offset + dims) {
			
			if (glm::compMin(dims) <= Precision(0.0))
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
			if (glm::compMin(newDims) <= Precision(0.0))
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

	template <VectorN<2> VecT = glm::vec2>
	class AspectRatioViewport : public Viewport<VecT> {
	public:
		using Vec = typename Viewport<VecT>::Vec;
		using RectT = typename Viewport<VecT>::RectT;
		using Precision = typename Viewport<VecT>::Precision;

	protected:
		Vec m_alignment = Vec(Precision(0.5));
		Precision m_aspectRatio{1.0};

	public:
		explicit AspectRatioViewport(Vec dims, Precision aspectRatio, Vec alignment = Vec(Precision(0.5))):
			Viewport<Vec>(dims), m_alignment{} {

			setAspectRatio(aspectRatio);
			setAlignment(alignment);
			AspectRatioViewport::resize(dims);
		}

		[[nodiscard]]
		constexpr double getAspectRatio() const noexcept override  {
			return m_aspectRatio;
		}

		double setAspectRatio(Precision aspectRatio) {
			if (aspectRatio <= Precision(0.0))
				throw std::invalid_argument("Aspect Ratio must be > 0.0");

			return std::exchange(m_aspectRatio, aspectRatio);
		}

		double setAspectRatio(Precision aspectRatio, Vec dims) {
			auto tmp = setAspectRatio(aspectRatio);
			resize(dims);

			return tmp;
		}

		Vec setAlignment(Vec alignment) {
			if (glm::compMin(alignment) < Precision(0.0))
				throw std::invalid_argument("Alignment must be above or equal to 0.0");

			if (glm::compMax(alignment) > Precision(1.0))
				throw std::invalid_argument("Alignment must be below or equal to 1.0");

			return std::exchange(m_alignment, alignment);
		}

		Vec setAlignment(Vec alignment, Vec dims) {
			auto tmp = setAlignment(alignment);
			resize(dims);

			return tmp;
		}

		virtual void resize(Vec newDims) override {
			if (glm::compMin(newDims) <= Precision(0.0))
				throw std::invalid_argument("Dimensions of Viewport must be > 0");

			auto asWidth = std::max(newDims.y * m_aspectRatio, Precision(1.0));

			if (asWidth == newDims.x) {
				this->m_rect.setPosDims(Vec(Precision(0.0)), newDims);
				return;
			}

			auto asHeight = std::max(newDims.x / m_aspectRatio, Precision(1.0));

			if (newDims.x > asWidth) {
				auto padding = (newDims.x - asWidth) * m_alignment.x;
				this->m_rect.setPosDims(Vec(padding, Precision(0.0)), Vec(asWidth, newDims.y));
				return;
			}

			auto padding = (newDims.y - asHeight) * m_alignment.y;
			this->m_rect.setPosDims(Vec(Precision(0.0), padding), Vec(newDims.x, asHeight));
		}
	};
}