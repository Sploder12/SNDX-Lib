#pragma once

#include <ft2build.h>
#include <freetype/freetype.h>

#include <glm/glm.hpp>

#include <optional>
#include <filesystem>

#include "atlas.hpp"

namespace sndx::render {
	struct GlyphMetric {
		glm::ivec2 bearing, dims;
		float advance;

		constexpr GlyphMetric() = default;

		constexpr GlyphMetric(float advance, const glm::ivec2& bearing, const glm::ivec2& dims) :
			bearing(bearing), dims(dims), advance(advance) {}
	};

	template <class AtlasT = TextureAtlas<FT_ULong>> 
	class Font {
	private:
		AtlasT m_atlas;
		std::unordered_map<FT_ULong, GlyphMetric> m_metrics{};

		int m_maxBearingY = 0;
		bool m_sdf{};

		friend class FontBuilder;

		Font(AtlasT&& atlas, const std::unordered_map<FT_ULong, GlyphMetric>& metrics, int maxBearingY, bool sdf) :
			m_atlas(std::move(atlas)), m_metrics(metrics), m_maxBearingY(maxBearingY), m_sdf(sdf) {}
	
	public:
		// returns charcode if it exists, the charcode "white square" otherwise
		[[nodiscard]]
		FT_ULong getChar(FT_ULong charcode) const {
			if (!contains(charcode)) return 9633;

			return charcode;
		}

		[[nodiscard]]
		bool contains(FT_ULong charcode) const {
			return m_metrics.contains(charcode);
		}

		[[nodiscard]]
		const GlyphMetric& getCharMetrics(FT_ULong charcode) const {
			return m_metrics.at(getChar(charcode));
		}

		[[nodiscard]]
		const auto& getAtlas() const {
			return m_atlas;
		}
	};

	class FontBuilder {
	private:
		using MetricsT = std::unordered_map<FT_ULong, GlyphMetric>;
		using GlyphsT = std::vector<std::pair<FT_ULong, ImageData>>;

		MetricsT m_metrics{};
		GlyphsT m_glyphs{};
		AtlasBuilder<FT_ULong> m_builder{};
		int m_maxBearingY;
		bool m_sdf;

		using DefaultPacker = decltype(m_builder)::DefaultPacker;

		friend class FreetypeContext;

		FontBuilder(MetricsT&& metrics, GlyphsT&& glyphs, int maxBearingY, bool sdf) :
			m_metrics(std::move(metrics)), m_glyphs(std::move(glyphs)), m_maxBearingY(maxBearingY), m_sdf(sdf) {

			m_builder.reserve(m_glyphs.size());

			for (const auto& [chr, glyph] : m_glyphs) {
				m_builder.add(chr, glyph);
			}
		}
	public:
		template <class Packer = DefaultPacker> [[nodiscard]]
		Font<ImageAtlas<FT_ULong>> build(auto&& policy, size_t dimConstraint, size_t padding) const {
			auto atlas = m_builder.build<Packer>(std::forward<decltype(policy)>(policy), dimConstraint, padding);

			return Font<ImageAtlas<FT_ULong>>{std::move(atlas), m_metrics, m_maxBearingY, m_sdf};
		}

		template <class Packer = DefaultPacker> [[nodiscard]]
		Font<ImageAtlas<FT_ULong>> build(size_t dimConstraint, size_t padding = 1) const {
			return build<Packer>(std::execution::par_unseq, dimConstraint, padding);
		}

		template <class TextureT, class Packer = DefaultPacker> [[nodiscard]]
		auto buildTexture(auto&& policy, size_t dimConstraint, size_t padding = 1, bool compress = false) {
			auto atlas = TextureAtlas<TextureT, FT_ULong>{m_builder.build<Packer>(std::forward<decltype(policy)>(policy), dimConstraint, padding), compress};
		
			return Font<TextureAtlas<TextureT, FT_ULong>>{std::move(atlas), m_metrics, m_maxBearingY, m_sdf};
		}

		template <class TextureT, class Packer = DefaultPacker> [[nodiscard]]
		auto buildTexture(size_t dimConstraint, size_t padding = 1, bool compress = false) {
			return buildTexture<TextureT, Packer>(std::execution::par_unseq, dimConstraint, padding, compress);
		}
	};

	struct FreetypeContext {
		FT_Library context = nullptr;

		FreetypeContext() {
			FT_Init_FreeType(&context);
		}

		operator FT_Library& () {
			return context;
		}

		FreetypeContext(const FreetypeContext&) = delete;
		FreetypeContext(FreetypeContext&& other) noexcept :
			context(std::exchange(other.context, nullptr)) {
		}

		FreetypeContext& operator=(const FreetypeContext&) = delete;
		FreetypeContext& operator=(FreetypeContext&& other) noexcept {
			std::swap(context, other.context);
			return *this;
		}

		~FreetypeContext() {
			FT_Done_FreeType(std::exchange(context, nullptr));
		}

		[[nodiscard]]
		std::optional<FontBuilder> loadFont(const char* filepath, unsigned int size = 32, bool sdf = false) {
			FT_Face face;
			if (FT_New_Face(context, filepath, 0, &face)) return {};

			// Set the pixel height to the size, the width is derived
			FT_Set_Pixel_Sizes(face, 0, size);

			FT_ULong count = face->num_glyphs;
			unsigned int columns = (unsigned int)(ceil(sqrt(count)));

			std::unordered_map<FT_ULong, GlyphMetric> metrics{};
			metrics.reserve(count);

			std::vector<std::pair<FT_ULong, ImageData>> glyphs{};
			glyphs.reserve(count);

			int maxBearingY = 0;
			
			// Iterate over each character
			FT_UInt idx;
			FT_ULong chr = FT_Get_First_Char(face, &idx);
			while (idx != 0) {
				FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT);

				const FT_GlyphSlot& glyphSlot = face->glyph;

				// Have freetype render the glyph
				if (FT_Render_Glyph(glyphSlot, (sdf) ? FT_RENDER_MODE_SDF : FT_RENDER_MODE_NORMAL)) {
					FT_Done_Face(face);
					return {};
				}

				const FT_Bitmap& bitmap = glyphSlot->bitmap;

				ImageData glyph{ bitmap.width, bitmap.rows, 1, std::span{ (const std::byte*)(bitmap.buffer), bitmap.width * bitmap.rows } };

				glyphs.emplace_back(chr, std::move(glyph));
		
				// Needed for proper text y alignment on the baseline
				maxBearingY = std::max(maxBearingY, glyphSlot->bitmap_top);

				// create the metric for the font, the advance is in 64ths of pixels so divide by 64 to get it into pixels
				metrics[chr] = GlyphMetric(glyphSlot->advance.x / 64.0f, glm::ivec2{ glyphSlot->bitmap_left, glyphSlot->bitmap_top }, glm::ivec2{ bitmap.width, bitmap.rows });

				chr = FT_Get_Next_Char(face, chr, &idx);
			}
			FT_Done_Face(face);

			return FontBuilder(std::move(metrics), std::move(glyphs), maxBearingY, sdf);
		}
	};
}