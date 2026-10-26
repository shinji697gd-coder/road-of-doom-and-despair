#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Popup.hpp>
#include <algorithm>

namespace roadui {
using namespace geode::prelude;

inline constexpr ccColor4B background{26, 30, 33, 255};
inline constexpr ccColor4B surface{37, 43, 47, 255};
inline constexpr ccColor4B inset{29, 34, 37, 255};
inline constexpr ccColor4B outline{67, 78, 83, 255};
inline constexpr ccColor4B buttonFill{49, 59, 64, 255};
inline constexpr ccColor4B selectedFill{47, 99, 69, 255};
inline constexpr ccColor3B text{235, 241, 238};
inline constexpr ccColor3B muted{165, 181, 184};
inline constexpr ccColor3B green{135, 221, 160};

inline CCLayerColor* rect(CCNode* parent, float x, float y, float w, float h,
                         ccColor4B color) {
    auto node = CCLayerColor::create(color, w, h);
    node->setPosition({x, y});
    parent->addChild(node);
    return node;
}

inline CCNode* panel(CCNode* parent, float x, float y, float w, float h,
                     ccColor4B fill = surface, ccColor4B border = outline) {
    auto node = CCNode::create();
    node->setContentSize({w, h});
    node->setPosition({x, y});
    rect(node, 0, 0, w, h, border);
    rect(node, 1, 1, w - 2, h - 2, fill);
    parent->addChild(node);
    return node;
}

inline CCLabelBMFont* label(CCNode* parent, std::string const& value,
                            float x, float y, float width, float scale = .36f,
                            ccColor3B color = text, bool left = false) {
    auto node = CCLabelBMFont::create(value.c_str(), "bigFont.fnt");
    node->setAnchorPoint({left ? 0.f : .5f, .5f});
    node->setPosition({x, y});
    node->limitLabelWidth(width, scale, .12f);
    node->setColor(color);
    parent->addChild(node);
    return node;
}

inline CCMenu* menu(CCNode* parent) {
    auto node = CCMenu::create();
    node->setPosition({0, 0});
    parent->addChild(node, 5);
    return node;
}

inline CCMenuItemSpriteExtra* button(CCMenu* parent, CCObject* target,
                                    SEL_MenuHandler callback,
                                    std::string const& caption,
                                    float x, float y, float w, float h = 24.f,
                                    bool selected = false) {
    auto image = CCNode::create();
    image->setContentSize({w, h});
    auto border = rect(image, 0, 0, w, h, selected ? ccColor4B{84, 157, 106, 255} : outline);
    border->setID("button-border");
    auto fill = rect(image, 1, 1, w - 2, h - 2, selected ? selectedFill : buttonFill);
    fill->setID("button-fill");
    auto title = label(image, caption, w / 2, h / 2, w - 12, .34f);
    title->setID("button-caption");
    auto item = CCMenuItemSpriteExtra::create(image, target, callback);
    item->setPosition({x, y});
    parent->addChild(item);
    return item;
}

inline void updateButton(CCMenuItemSpriteExtra* button,
                         std::string const& caption, bool selected = false) {
    auto image = button->getNormalImage();
    auto fill = static_cast<CCLayerColor*>(image->getChildByID("button-fill"));
    const auto color = selected ? selectedFill : buttonFill;
    fill->setColor({color.r, color.g, color.b});
    auto title = static_cast<CCLabelBMFont*>(image->getChildByID("button-caption"));
    title->setString(caption.c_str());
    title->limitLabelWidth(image->getContentWidth() - 12, .34f, .12f);
    auto border = static_cast<CCLayerColor*>(image->getChildByID("button-border"));
    border->setColor(selected ? ccColor3B{84, 157, 106} : ccColor3B{outline.r, outline.g, outline.b});
}

inline TextInput* input(CCNode* parent, float x, float y, float width,
                         std::string const& placeholder,
                         CommonFilter filter = CommonFilter::Any) {
    constexpr float scale = .8f;
    auto node = TextInput::create(width / scale, placeholder);
    node->setScale(scale);
    node->setPosition({x, y});
    node->setTextAlign(TextInputAlign::Left);
    node->setCommonFilter(filter);
    node->getInputNode()->setLabelPlaceholderScale(.4f);
    node->getInputNode()->setMaxLabelScale(.45f);
    // Use our own background so a texture pack cannot resize an input frame.
    node->hideBG();
    panel(parent, x - width / 2, y - 12, width, 24, inset);
    parent->addChild(node, 3);
    return node;
}

inline void fitPopup(CCNode* window, float width, float height) {
    const auto screen = CCDirector::get()->getWinSize();
    window->setScale(std::min({1.f, (screen.width - 20.f) / width,
                                     (screen.height - 16.f) / height}));
}
}
