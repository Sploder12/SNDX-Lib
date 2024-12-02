#pragma once

#include "../math/binpack.hpp"

#include "./image/imagedata.hpp"

#include <unordered_map>
#include <string>
#include <functional>
#include <algorithm>
#include <execution>

namespace sndx::render {
	
	template <class IdT = std::string>
	class ImageAtlas {
	private:
		struct Entry {
			glm::vec<2, size_t> pos, dims;
		};

		std::unordered_map<IdT, Entry> m_entries{};
		ImageData m_image;

		template <class>
		friend class AtlasBuilder;

		ImageAtlas(decltype(m_entries)&& entries, ImageData&& image) :
			m_entries(std::move(entries)), m_image(std::move(image)) {}
	public:

		[[nodiscard]]
		const auto& getImage() const {
			return m_image;
		}

		[[nodiscard]]
		const auto& getEntry(const IdT& id) const {
			return m_entries.at(id);
		}

		[[nodiscard]]
		auto size() const {
			return m_entries.size();
		}

		[[nodiscard]]
		auto begin() const {
			return m_entries.begin();
		}

		[[nodiscard]]
		auto end() const {
			return m_entries.end();
		}
	};

	template <class TextureT, class IdT = std::string>
	class TextureAtlas {
	private:
		struct Entry {
			glm::vec2 pos, dims;
		};

		std::unordered_map<IdT, Entry> m_entries;
		TextureT m_texture;

		TextureAtlas(decltype(m_entries)&& entries, TextureT&& texture) :
			m_entries(std::move(entries)), m_texture(std::move(texture)) {}
	public:

		TextureAtlas(const ImageAtlas<IdT>& atlas, bool compress = false):
			m_entries{}, m_texture{atlas.getImage(), 0, compress } {
			m_entries.reserve(atlas.size());
			const auto& image = atlas.getImage();

			glm::vec2 scaling = 1.0f / glm::vec2{ image.width(), image.height()};

			for (const auto& [id, entry] : atlas) {
				Entry e{ glm::vec2{entry.pos} *scaling, glm::vec2{entry.dims} *scaling };
				m_entries.emplace(id, std::move(e));
			}
		}
		
		[[nodiscard]]
		const auto& getTexture() const {
			return m_texture;
		}

		[[nodiscard]]
		const auto& getEntry(const IdT& id) const {
			return m_entries.at(id);
		}

		[[nodiscard]]
		auto size() const {
			return m_entries.size();
		}

		[[nodiscard]]
		auto begin() const {
			return m_entries.begin();
		}

		[[nodiscard]]
		auto end() const {
			return m_entries.end();
		}
	};

	template <class IdT = std::string>
	class AtlasBuilder {
	private:
		struct Entry {
			IdT id;
			std::reference_wrapper<const ImageData> data;
		};

		std::vector<Entry> m_entries{};
		bool m_compress = false;

	public:
		using DefaultPacker = sndx::math::BinPacker<true, size_t>;

		void add(const IdT& id, const ImageData& img) {
			m_entries.emplace_back(id, img);
		}

		void reserve(size_t size) noexcept {
			m_entries.reserve(size);
		}

		template <class Packer = DefaultPacker> [[nodiscard]]
		ImageAtlas<IdT> build(auto&& policy, size_t dimConstraint, size_t padding) const {
			Packer packer{};

			size_t maxChannels = 0;
			for (size_t i = 0; i < m_entries.size(); ++i) {
				const auto& img = m_entries[i].data.get();

				maxChannels = std::max(maxChannels, size_t(img.channels()));
				packer.add(i, img.width(), img.height());
			}

			auto packing = packer.pack(dimConstraint, padding);
			if (packing.empty() || packing.width() == 0 || packing.height() == 0) [[unlikely]] {
				throw std::logic_error("Cannot create an empty atlas");
			}
			
			std::vector<std::byte> data{};

			data.resize(maxChannels * (packing.width() + padding) * (packing.height() + padding), std::byte(0x0));

			std::for_each_n(std::forward<decltype(policy)>(policy), packing.begin(), m_entries.size(), 
				[this, &data, &maxChannels, &packing, &padding](const auto& entry) {
				
				const auto& [imgIdx, pos] = entry;
				const auto& [id, imgRef] = m_entries[imgIdx];
				const auto& img = imgRef.get();
				
				size_t stride = maxChannels * (packing.width() + padding);

				for (size_t y = 0; y < img.height(); ++y) {
					size_t rowPos = (pos.y + y) * stride + pos.x * maxChannels;

					for (size_t x = 0; x < img.width(); ++x) {
						
						for (size_t c = 0; c < img.channels(); ++c) {
							data.at(rowPos + x * maxChannels + c) = img.at(x, y, c);
						}

						for (size_t c = img.channels(); c < maxChannels; ++c) {
							data.at(rowPos + x * maxChannels + c) = c >= 3 ? std::byte(0xff) : std::byte(0x0);
						}
					}
				}
			});

			std::unordered_map<IdT, typename ImageAtlas<IdT>::Entry> entries{};
			entries.reserve(m_entries.size());

			for (const auto& [imgIdx, pos] : packing) {
				const auto& [id, img] = m_entries[imgIdx];

				typename ImageAtlas<IdT>::Entry entry{
					pos,
					glm::vec<2, size_t>{img.get().width(), img.get().height()}
				};

				entries.emplace(id, std::move(entry));
			}

			return ImageAtlas<IdT>{ std::move(entries), ImageData{packing.width() + padding, packing.height() + padding, uint8_t(maxChannels), std::move(data)}};
		}

		template <class Packer = DefaultPacker> [[nodiscard]]
		ImageAtlas<IdT> build(size_t dimConstraint, size_t padding = 1) const {
			return build<Packer>(std::execution::par_unseq, dimConstraint, padding);
		}

		template <class TextureT, class Packer = DefaultPacker> [[nodiscard]]
		auto buildTexture(auto&& policy, size_t dimConstraint, size_t padding = 1, bool compress = false) {
			return TextureAtlas<TextureT, IdT>{build<Packer>(std::forward<decltype(policy)>(policy), dimConstraint, padding), compress};
		}

		template <class TextureT, class Packer = DefaultPacker> [[nodiscard]]
		auto buildTexture(size_t dimConstraint, size_t padding = 1, bool compress = false) {
			return buildTexture<TextureT, Packer>(std::execution::par_unseq, dimConstraint, padding, compress);
		}
	};
}