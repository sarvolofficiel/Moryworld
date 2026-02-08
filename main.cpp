// ============================================================================
// SOUL WORLD - Infinite Waves Edition (Menu Centré)
// ============================================================================

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <random>
#include <cmath>
#include <iostream>
#include <optional>
#include <algorithm>
#include <functional>

// ============================================================================
// CONFIGURATION
// ============================================================================

namespace Config {
    constexpr unsigned int WINDOW_WIDTH = 1920;
    constexpr unsigned int WINDOW_HEIGHT = 1080;
    constexpr float GRAVITY = 980.f;
    constexpr float PLAYER_SPEED = 350.f;
    constexpr float JUMP_FORCE = 520.f;
    constexpr float DASH_SPEED = 800.f;
    constexpr float DASH_DURATION = 0.15f;
    constexpr float DASH_COOLDOWN = 0.8f;
    constexpr float COYOTE_TIME = 0.1f;
    constexpr float JUMP_BUFFER_TIME = 0.1f;
    constexpr float FLIGHT_DURATION = 5.0f;
    constexpr int MAX_HEALTH = 5;
    constexpr int MAX_SOUL_ENERGY = 100;
    constexpr float ENEMY_FLY_DURATION = 7.0f;
    constexpr float ENEMY_FLY_COOLDOWN = 5.0f;
}

// ============================================================================
// GESTIONNAIRE DE POLICE
// ============================================================================

class FontManager {
public:
    static FontManager& instance() {
        static FontManager inst;
        return inst;
    }

    bool loadFont() {
        std::vector<std::string> fontPaths = {
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/Arial.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
            "/usr/share/fonts/gnu-free/FreeSans.ttf",
            "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/TTF/LiberationSans-Regular.ttf",
            "/usr/share/fonts/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/TTF/Roboto-Regular.ttf",
            "/usr/share/fonts/noto/NotoSans-Medium.ttf",
            "/usr/share/fonts/adobe-source-code-pro/SourceCodePro-Regular.otf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "./assets/fonts/font.ttf",
            "./font.ttf",
            "font.ttf"
        };

        for (const auto& path : fontPaths) {
            if (font.openFromFile(path)) {
                std::cout << "Police chargee: " << path << std::endl;
                fontLoaded = true;
                return true;
            }
        }

        std::cerr << "ATTENTION: Aucune police trouvee!" << std::endl;
        fontLoaded = false;
        return false;
    }

    sf::Font& getFont() { return font; }
    bool isLoaded() const { return fontLoaded; }

private:
    sf::Font font;
    bool fontLoaded = false;
};

// ============================================================================
// UTILITAIRES
// ============================================================================

namespace Math {
    inline float lerp(float a, float b, float t) {
        return a + (b - a) * std::clamp(t, 0.f, 1.f);
    }

    inline sf::Vector2f lerp(sf::Vector2f a, sf::Vector2f b, float t) {
        return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)};
    }

    inline float length(sf::Vector2f v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    inline sf::Vector2f normalize(sf::Vector2f v) {
        float len = length(v);
        return len > 0 ? v / len : sf::Vector2f{0, 0};
    }

    inline float distance(sf::Vector2f a, sf::Vector2f b) {
        return length(b - a);
    }
}

class Random {
public:
    static Random& instance() {
        static Random inst;
        return inst;
    }

    float range(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    int range(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }

    sf::Vector2f insideCircle(float radius) {
        float angle = range(0.f, 6.28318f);
        float r = range(0.f, radius);
        return {std::cos(angle) * r, std::sin(angle) * r};
    }

private:
    std::mt19937 gen{std::random_device{}()};
};

// ============================================================================
// PARTICULES
// ============================================================================

struct ParticleConfig {
    sf::Color startColor{255, 255, 255, 255};
    sf::Color endColor{255, 255, 255, 0};
    float minSpeed = 50.f, maxSpeed = 150.f;
    float minLife = 0.5f, maxLife = 1.5f;
    float minSize = 3.f, maxSize = 8.f;
    float endSize = 0.f;
    float direction = -1.57f;
    float spread = 0.5f;
    float spawnRadius = 5.f;
    float rotationSpeed = 2.f;
};

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color, endColor;
    float life, maxLife;
    float size, endSize;
    float rotation, rotationSpeed;
    bool active = false;
};

class ParticleSystem {
public:
    ParticleSystem(size_t maxParticles = 2000) : particles(maxParticles) {
        vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
        vertices.resize(maxParticles * 6);
    }

    void emit(sf::Vector2f pos, const ParticleConfig& cfg, int count = 1) {
        for (int i = 0; i < count; ++i) {
            for (auto& p : particles) {
                if (!p.active) {
                    p.active = true;
                    p.position = pos + Random::instance().insideCircle(cfg.spawnRadius);
                    float angle = cfg.direction + Random::instance().range(-cfg.spread, cfg.spread);
                    float speed = Random::instance().range(cfg.minSpeed, cfg.maxSpeed);
                    p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
                    p.life = p.maxLife = Random::instance().range(cfg.minLife, cfg.maxLife);
                    p.color = cfg.startColor;
                    p.endColor = cfg.endColor;
                    p.size = Random::instance().range(cfg.minSize, cfg.maxSize);
                    p.endSize = cfg.endSize;
                    p.rotation = Random::instance().range(0.f, 6.28f);
                    p.rotationSpeed = Random::instance().range(-cfg.rotationSpeed, cfg.rotationSpeed);
                    break;
                }
            }
        }
    }

    void update(float dt) {
        size_t idx = 0;
        for (auto& p : particles) {
            if (!p.active) continue;
            p.life -= dt;
            if (p.life <= 0) { p.active = false; continue; }

            float ratio = p.life / p.maxLife;
            p.velocity.y += gravity * dt;
            p.velocity *= 1.f - drag * dt;
            p.position += p.velocity * dt;
            p.rotation += p.rotationSpeed * dt;

            float t = 1.f - ratio;
            sf::Color col(
                uint8_t(Math::lerp(float(p.color.r), float(p.endColor.r), t)),
                          uint8_t(Math::lerp(float(p.color.g), float(p.endColor.g), t)),
                          uint8_t(Math::lerp(float(p.color.b), float(p.endColor.b), t)),
                          uint8_t(float(p.color.a) * ratio)
            );

            float sz = Math::lerp(p.size, p.endSize, t);
            float c = std::cos(p.rotation) * sz, s = std::sin(p.rotation) * sz;

            sf::Vector2f corners[4] = {{-c+s,-s-c}, {c+s,s-c}, {c-s,s+c}, {-c-s,-s+c}};
            vertices[idx++] = {p.position + corners[0], col};
            vertices[idx++] = {p.position + corners[1], col};
            vertices[idx++] = {p.position + corners[2], col};
            vertices[idx++] = {p.position + corners[0], col};
            vertices[idx++] = {p.position + corners[2], col};
            vertices[idx++] = {p.position + corners[3], col};
        }
        activeVerts = idx;
    }

    void draw(sf::RenderTarget& target) const {
        if (activeVerts > 0) {
            sf::VertexArray vis(sf::PrimitiveType::Triangles, activeVerts);
            for (size_t i = 0; i < activeVerts; ++i) vis[i] = vertices[i];
            target.draw(vis, sf::RenderStates(blendMode));
        }
    }

    float gravity = 200.f, drag = 0.5f;
    sf::BlendMode blendMode = sf::BlendAdd;

private:
    std::vector<Particle> particles;
    sf::VertexArray vertices;
    size_t activeVerts = 0;
};

// ============================================================================
// INPUT
// ============================================================================

class InputManager {
public:
    void update() {
        prevState = currState;
        updateKey("left", sf::Keyboard::Key::Left, sf::Keyboard::Key::A);
        updateKey("right", sf::Keyboard::Key::Right, sf::Keyboard::Key::D);
        updateKey("up", sf::Keyboard::Key::Up, sf::Keyboard::Key::W);
        updateKey("down", sf::Keyboard::Key::Down, sf::Keyboard::Key::S);
        updateKey("jump", sf::Keyboard::Key::Space, sf::Keyboard::Key::Space);
        updateKey("dash", sf::Keyboard::Key::LShift, sf::Keyboard::Key::K);
        updateKey("attack", sf::Keyboard::Key::V, sf::Keyboard::Key::V);
        updateKey("y", sf::Keyboard::Key::Y, sf::Keyboard::Key::Y);
        updateKey("h", sf::Keyboard::Key::H, sf::Keyboard::Key::H);
        updateKey("p", sf::Keyboard::Key::P, sf::Keyboard::Key::P);
        updateKey("pause", sf::Keyboard::Key::Escape, sf::Keyboard::Key::Escape);
        updateKey("confirm", sf::Keyboard::Key::Enter, sf::Keyboard::Key::Enter);
        updateKey("back", sf::Keyboard::Key::Escape, sf::Keyboard::Key::Backspace);
        updateKey("num1", sf::Keyboard::Key::Num1, sf::Keyboard::Key::Numpad1);
        updateKey("num2", sf::Keyboard::Key::Num2, sf::Keyboard::Key::Numpad2);
        updateKey("num3", sf::Keyboard::Key::Num3, sf::Keyboard::Key::Numpad3);
    }

    bool isPressed(const std::string& a) const {
        auto it = currState.find(a);
        return it != currState.end() && it->second;
    }

    bool justPressed(const std::string& a) const {
        auto c = currState.find(a), p = prevState.find(a);
        return (c != currState.end() && c->second) &&
        (p == prevState.end() || !p->second);
    }

    bool justReleased(const std::string& a) const {
        auto c = currState.find(a), p = prevState.find(a);
        return (c == currState.end() || !c->second) &&
        (p != prevState.end() && p->second);
    }

    float getAxis(const std::string& neg, const std::string& pos) const {
        float v = 0;
        if (isPressed(neg)) v -= 1.f;
        if (isPressed(pos)) v += 1.f;
        return v;
    }

private:
    void updateKey(const std::string& a, sf::Keyboard::Key k1, sf::Keyboard::Key k2) {
        currState[a] = sf::Keyboard::isKeyPressed(k1) || sf::Keyboard::isKeyPressed(k2);
    }
    std::unordered_map<std::string, bool> currState, prevState;
};

// ============================================================================
// PROJECTILE
// ============================================================================

class Projectile {
public:
    Projectile(sf::Vector2f pos, sf::Vector2f dir, float speed, sf::Color color)
    : position(pos), velocity(Math::normalize(dir) * speed), baseColor(color) {
        shape.setRadius(initialRadius);
        shape.setOrigin({initialRadius, initialRadius});
        shape.setFillColor(color);
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(sf::Color(color.r, color.g, color.b, 150));
    }

    void update(float dt) {
        if (!active) return;

        lifetime += dt;
        position += velocity * dt;

        float growthFactor = 1.f + lifetime * growthRate;
        currentRadius = std::min(initialRadius * growthFactor, maxRadius);

        shape.setRadius(currentRadius);
        shape.setOrigin({currentRadius, currentRadius});
        shape.setPosition(position);

        rotation += velocity.x * dt * 0.1f;
        shape.setRotation(sf::degrees(rotation));

        if (int(lifetime * 20) % 2 == 0) {
            trailPositions.push_back(position);
            if (trailPositions.size() > 10) trailPositions.erase(trailPositions.begin());
        }

        if (position.x < -100 || position.x > 3200 || position.y > 1200) {
            active = false;
        }
    }

    void draw(sf::RenderTarget& target) const {
        if (!active) return;

        for (size_t i = 0; i < trailPositions.size(); ++i) {
            float alpha = float(i) / trailPositions.size() * 100.f;
            float radius = currentRadius * 0.3f * (float(i) / trailPositions.size());
            sf::CircleShape trail(radius);
            trail.setOrigin({radius, radius});
            trail.setPosition(trailPositions[i]);
            trail.setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b, uint8_t(alpha)));
            target.draw(trail);
        }

        target.draw(shape);

        sf::CircleShape glow(currentRadius * 1.3f);
        glow.setOrigin({currentRadius * 1.3f, currentRadius * 1.3f});
        glow.setPosition(position);
        glow.setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b, 30));
        target.draw(glow, sf::RenderStates(sf::BlendAdd));
    }

    sf::FloatRect getBounds() const {
        return {{position.x - currentRadius, position.y - currentRadius},
        {currentRadius * 2, currentRadius * 2}};
    }

    bool isActive() const { return active; }
    void deactivate() { active = false; }
    float getDamage() const { return 1.f + (currentRadius / maxRadius); }

private:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::CircleShape shape;
    sf::Color baseColor;

    float initialRadius = 12.f;
    float currentRadius = 12.f;
    float maxRadius = 50.f;
    float growthRate = 0.8f;
    float lifetime = 0;
    float rotation = 0;
    bool active = true;

    std::vector<sf::Vector2f> trailPositions;
};

// ============================================================================
// PLATEFORME
// ============================================================================

class Platform {
public:
    Platform(sf::Vector2f size, sf::Vector2f pos, bool oneWay = false)
    : isOneWay(oneWay), originalPos(pos), previousPos(pos) {
        shape.setSize(size);
        shape.setPosition(pos);
        shape.setFillColor(oneWay ? sf::Color(55, 45, 65, 200) : sf::Color(35, 30, 45, 255));
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(sf::Color(90, 80, 110, 150));

        highlight.setSize({size.x, 3.f});
        highlight.setPosition(pos);
        highlight.setFillColor(sf::Color(140, 130, 170, 100));

        bounds = shape.getGlobalBounds();
    }

    void update(float dt) {
        previousPos = shape.getPosition();

        if (isMoving) {
            moveTimer += dt * moveSpeed;
            float offset = std::sin(moveTimer) * moveRange;
            sf::Vector2f newPos = originalPos;
            if (moveHorizontal) newPos.x += offset;
            else newPos.y += offset;
            shape.setPosition(newPos);
            highlight.setPosition(newPos);
            bounds = shape.getGlobalBounds();
            velocity = (shape.getPosition() - previousPos) / dt;
        }
    }

    void draw(sf::RenderTarget& target) const {
        target.draw(shape);
        target.draw(highlight);
    }

    sf::FloatRect getBounds() const { return bounds; }
    bool getIsOneWay() const { return isOneWay; }
    sf::Vector2f getVelocity() const { return velocity; }

    void setMoving(bool horizontal, float range, float speed) {
        isMoving = true;
        moveHorizontal = horizontal;
        moveRange = range;
        moveSpeed = speed;
    }

private:
    sf::RectangleShape shape, highlight;
    sf::FloatRect bounds;
    bool isOneWay;
    sf::Vector2f originalPos, previousPos;
    sf::Vector2f velocity{0, 0};
    bool isMoving = false;
    bool moveHorizontal = true;
    float moveRange = 0, moveSpeed = 1.f, moveTimer = 0;
};

// ============================================================================
// STATS DU JOUEUR
// ============================================================================

struct PlayerStats {
    float speedMultiplier = 1.f;
    float jumpMultiplier = 1.f;
    float dashCooldownMultiplier = 1.f;
    float flightDurationMultiplier = 1.f;
    int maxHealth = Config::MAX_HEALTH;
    float attackRange = 40.f;
    float attackSpeed = 1.f;
    int attackDamage = 1;

    void reset() {
        speedMultiplier = jumpMultiplier = 1.f;
        dashCooldownMultiplier = flightDurationMultiplier = attackSpeed = 1.f;
        maxHealth = Config::MAX_HEALTH;
        attackRange = 40.f;
        attackDamage = 1;
    }
};

// ============================================================================
// JOUEUR
// ============================================================================

class Player {
public:
    enum class State { Idle, Running, Jumping, Falling, Dashing, Attacking, Hurt, Dead, Flying };

    Player(sf::Vector2f startPos) : position(startPos) {
        body.setRadius(18.f);
        body.setOrigin({18.f, 18.f});
        body.setFillColor(sf::Color(180, 220, 255, 230));
        body.setOutlineThickness(2.f);
        body.setOutlineColor(sf::Color(100, 150, 200, 200));

        glow.setRadius(25.f);
        glow.setOrigin({25.f, 25.f});
        glow.setFillColor(sf::Color(150, 200, 255, 50));

        eye.setRadius(4.f);
        eye.setOrigin({4.f, 4.f});
        eye.setFillColor(sf::Color(50, 50, 80));

        trail.gravity = 50.f;
        trail.drag = 2.f;
    }

    void handleInput(const InputManager& input, float dt) {
        if (state == State::Dead || state == State::Hurt) return;

        float moveInput = input.getAxis("left", "right");

        if (state != State::Dashing) {
            if (std::abs(moveInput) > 0.1f) facingRight = moveInput > 0;

            if (state != State::Flying) {
                float targetSpeed = moveInput * Config::PLAYER_SPEED * stats.speedMultiplier;
                velocity.x = Math::lerp(velocity.x, targetSpeed, isGrounded ? 15.f * dt : 8.f * dt);
            }
        }

        coyoteTimer = isGrounded ? Config::COYOTE_TIME : coyoteTimer - dt;
        jumpBufferTimer = input.justPressed("jump") ? Config::JUMP_BUFFER_TIME : jumpBufferTimer - dt;

        if (jumpBufferTimer > 0 && coyoteTimer > 0 && state != State::Flying) {
            velocity.y = -Config::JUMP_FORCE * stats.jumpMultiplier;
            isGrounded = false;
            coyoteTimer = jumpBufferTimer = 0;
            createJumpParticles();
        }

        if (input.justReleased("jump") && velocity.y < 0) velocity.y *= 0.5f;
        if (input.justPressed("dash") && dashCooldown <= 0 && state != State::Flying) startDash();
        if (input.justPressed("attack") && state != State::Dashing && attackCooldown <= 0) startAttack();

        handleInvocation(input);
    }

    void handleInvocation(const InputManager& input) {
        invocationTimer -= 1.f/60.f;

        if (input.justPressed("y")) { invocationSeq = "Y"; invocationTimer = 2.f; }
        else if (input.justPressed("h") && invocationSeq == "Y" && invocationTimer > 0) invocationSeq = "YH";
        else if (input.justPressed("p") && invocationSeq == "YH" && invocationTimer > 0) activateFlight();

        if (invocationTimer <= 0) invocationSeq = "";
    }

    void activateFlight() {
        state = State::Flying;
        flightTimer = Config::FLIGHT_DURATION * stats.flightDurationMultiplier;
        invocationSeq = "";

        ParticleConfig cfg;
        cfg.startColor = sf::Color(100, 200, 255, 255);
        cfg.endColor = sf::Color(50, 100, 200, 0);
        cfg.minSpeed = 200.f; cfg.maxSpeed = 400.f;
        cfg.spread = 3.14159f;
        dragonFx.emit(position, cfg, 100);

        body.setFillColor(sf::Color(100, 200, 255, 255));
        glow.setRadius(40.f);
        glow.setOrigin({40.f, 40.f});
        glow.setFillColor(sf::Color(50, 150, 255, 100));
    }

    void update(float dt, const std::vector<Platform>& platforms) {
        if (dashCooldown > 0) dashCooldown -= dt;
        if (invincibility > 0) invincibility -= dt;
        if (attackTimer > 0) attackTimer -= dt;
        if (attackCooldown > 0) attackCooldown -= dt;
        if (hurtTimer > 0) {
            hurtTimer -= dt;
            if (hurtTimer <= 0 && state == State::Hurt) state = State::Idle;
        }

        if (state == State::Flying) {
            flightTimer -= dt;
            velocity.y = -150.f;
            velocity.x *= 0.95f;

            ParticleConfig cfg;
            cfg.startColor = sf::Color(100, 200, 255, 200);
            cfg.endColor = sf::Color(50, 100, 200, 0);
            cfg.direction = 1.57f; cfg.spread = 0.5f;
            cfg.minSpeed = 50.f; cfg.maxSpeed = 100.f;
            cfg.minLife = 0.3f; cfg.maxLife = 0.6f;
            trail.emit(position + sf::Vector2f{0, 15.f}, cfg, 2);

            if (flightTimer <= 0) {
                state = State::Falling;
                body.setFillColor(sf::Color(180, 220, 255, 230));
                glow.setRadius(25.f);
                glow.setOrigin({25.f, 25.f});
                glow.setFillColor(sf::Color(150, 200, 255, 50));
            }
        }

        if (state == State::Dashing) {
            dashTimer -= dt;
            if (dashTimer <= 0) {
                state = isGrounded ? State::Idle : State::Falling;
                velocity.x = facingRight ? 150.f : -150.f;
            }

            ParticleConfig cfg;
            cfg.startColor = sf::Color(200, 230, 255, 200);
            cfg.endColor = sf::Color(100, 150, 200, 0);
            cfg.minSpeed = 20.f; cfg.maxSpeed = 50.f;
            cfg.minLife = 0.15f; cfg.maxLife = 0.3f;
            cfg.minSize = 8.f; cfg.maxSize = 15.f;
            trail.emit(position, cfg, 3);
        }

        if (state != State::Dashing && state != State::Flying) {
            velocity.y = std::min(velocity.y + Config::GRAVITY * dt, 800.f);
        }

        position += velocity * dt;

        isGrounded = false;
        platformVelocity = {0, 0};

        for (const auto& plat : platforms) {
            if (resolveCollision(plat)) {
                if (isGrounded && (plat.getVelocity().x != 0 || plat.getVelocity().y != 0)) {
                    platformVelocity = plat.getVelocity();
                }
            }
        }

        if (isGrounded) position += platformVelocity * dt;

        position.x = std::clamp(position.x, 20.f, 3000.f);

        updateState();
        trail.update(dt);
        dragonFx.update(dt);

        body.setPosition(position);
        glow.setPosition(position);
        eye.setPosition(position + sf::Vector2f{facingRight ? 6.f : -6.f, -5.f});

        float breathe = 1.f + std::sin(animTimer * 3.f) * 0.05f;
        body.setScale({breathe, breathe});
        animTimer += dt;
    }

    bool resolveCollision(const Platform& plat) {
        sf::FloatRect pBounds = getCollisionBounds();
        sf::FloatRect tBounds = plat.getBounds();

        if (!pBounds.findIntersection(tBounds)) return false;

        float oL = (pBounds.position.x + pBounds.size.x) - tBounds.position.x;
        float oR = (tBounds.position.x + tBounds.size.x) - pBounds.position.x;
        float oT = (pBounds.position.y + pBounds.size.y) - tBounds.position.y;
        float oB = (tBounds.position.y + tBounds.size.y) - pBounds.position.y;

        float minX = oL < oR ? -oL : oR;
        float minY = oT < oB ? -oT : oB;

        if (plat.getIsOneWay()) {
            if (velocity.y > 0 && oT < 15.f) {
                position.y = tBounds.position.y - 18.f;
                velocity.y = 0;
                isGrounded = true;
                return true;
            }
            return false;
        }

        if (std::abs(minX) < std::abs(minY)) {
            position.x += minX;
            velocity.x = 0;
        } else {
            position.y += minY;
            if (minY < 0) isGrounded = true;
            velocity.y = 0;
        }
        return true;
    }

    sf::FloatRect getCollisionBounds() const {
        return {{position.x - 15.f, position.y - 15.f}, {30.f, 33.f}};
    }

    void startDash() {
        state = State::Dashing;
        dashTimer = Config::DASH_DURATION;
        dashCooldown = Config::DASH_COOLDOWN * stats.dashCooldownMultiplier;
        velocity = {facingRight ? Config::DASH_SPEED : -Config::DASH_SPEED, 0};
        invincibility = Config::DASH_DURATION;
    }

    void startAttack() {
        state = State::Attacking;
        attackTimer = 0.25f / stats.attackSpeed;
        attackCooldown = 0.4f / stats.attackSpeed;
        isAttacking = true;

        float range = stats.attackRange;
        attackBounds = {{position.x + (facingRight ? 15.f : -15.f - range), position.y - 20.f}, {range, 40.f}};

        ParticleConfig cfg;
        cfg.startColor = sf::Color(255, 255, 255, 200);
        cfg.endColor = sf::Color(200, 220, 255, 0);
        cfg.direction = facingRight ? 0.f : 3.14159f;
        cfg.spread = 0.5f;
        cfg.minSpeed = 300.f; cfg.maxSpeed = 500.f;
        cfg.minLife = 0.1f; cfg.maxLife = 0.2f;
        trail.emit(position + sf::Vector2f{facingRight ? 20.f : -20.f, 0}, cfg, 20);
    }

    void takeDamage(int dmg) {
        if (invincibility > 0 || state == State::Dead) return;
        health -= dmg;
        invincibility = 1.5f;
        state = State::Hurt;
        hurtTimer = 0.3f;
        velocity = {facingRight ? -200.f : 200.f, -150.f};

        ParticleConfig cfg;
        cfg.startColor = sf::Color(255, 100, 100, 255);
        cfg.endColor = sf::Color(100, 50, 50, 0);
        cfg.minSpeed = 100.f; cfg.maxSpeed = 300.f;
        cfg.spread = 3.14159f;
        trail.emit(position, cfg, 30);

        if (health <= 0) {
            state = State::Dead;
            cfg.startColor = sf::Color(180, 220, 255, 255);
            cfg.endColor = sf::Color(50, 100, 150, 0);
            cfg.minSpeed = 200.f; cfg.maxSpeed = 500.f;
            cfg.minLife = 1.f; cfg.maxLife = 2.f;
            trail.emit(position, cfg, 200);
        }
    }

    void heal(int amount) { health = std::min(health + amount, stats.maxHealth); }

    void createJumpParticles() {
        ParticleConfig cfg;
        cfg.startColor = sf::Color(200, 220, 255, 200);
        cfg.endColor = sf::Color(150, 180, 220, 0);
        cfg.direction = 1.57f; cfg.spread = 0.8f;
        cfg.minSpeed = 50.f; cfg.maxSpeed = 150.f;
        cfg.minLife = 0.2f; cfg.maxLife = 0.4f;
        trail.emit(position + sf::Vector2f{0, 15.f}, cfg, 15);
    }

    void updateState() {
        if (state == State::Dead || state == State::Dashing ||
            state == State::Flying || state == State::Hurt) return;

        if (attackTimer > 0) state = State::Attacking;
        else if (!isGrounded) state = velocity.y < 0 ? State::Jumping : State::Falling;
        else if (std::abs(velocity.x) > 20.f) state = State::Running;
        else state = State::Idle;

        isAttacking = attackTimer > 0;
    }

    void draw(sf::RenderTarget& target) const {
        trail.draw(target);
        dragonFx.draw(target);

        if (invincibility > 0 && int(invincibility * 10) % 2 == 0) return;

        target.draw(glow, sf::RenderStates(sf::BlendAdd));
        target.draw(body);
        target.draw(eye);

        if (isAttacking) {
            sf::RectangleShape slash{{stats.attackRange, 4.f}};
            slash.setOrigin({0.f, 2.f});
            slash.setPosition(position + sf::Vector2f{facingRight ? 18.f : -18.f, 0});
            slash.setRotation(sf::degrees(facingRight ? -20.f : 200.f));
            slash.setFillColor(sf::Color(255, 255, 255, 200));
            target.draw(slash);
        }

        if (!invocationSeq.empty()) {
            float progress = float(invocationSeq.length()) / 3.f;
            sf::CircleShape ind(5.f + 5.f * progress);
            ind.setOrigin({ind.getRadius(), ind.getRadius()});
            ind.setPosition(position + sf::Vector2f{0, -40.f});
            ind.setFillColor(sf::Color(uint8_t(100 + 155 * progress), 200, 255, 200));
            target.draw(ind);
        }
    }

    void reset(sf::Vector2f pos) {
        position = pos;
        velocity = {0, 0};
        health = stats.maxHealth;
        soulEnergy = 0;
        state = State::Idle;
        flightTimer = 0;
        invincibility = 0;
        body.setFillColor(sf::Color(180, 220, 255, 230));
        glow.setRadius(25.f);
        glow.setOrigin({25.f, 25.f});
    }

    void fullReset(sf::Vector2f pos) {
        reset(pos);
        stats.reset();
    }

    sf::Vector2f getPosition() const { return position; }
    int getHealth() const { return health; }
    int getSoulEnergy() const { return soulEnergy; }
    State getState() const { return state; }
    sf::FloatRect getAttackBounds() const { return attackBounds; }
    bool getIsAttacking() const { return isAttacking; }
    float getFlightTimer() const { return flightTimer; }
    PlayerStats& getStats() { return stats; }
    const PlayerStats& getStats() const { return stats; }

    void addSoul(int amt) { soulEnergy = std::min(soulEnergy + amt, Config::MAX_SOUL_ENERGY); }
    int getAttackDamage() const { return stats.attackDamage; }

private:
    sf::Vector2f position;
    sf::Vector2f velocity{0, 0};
    sf::Vector2f platformVelocity{0, 0};

    sf::CircleShape body, glow, eye;

    State state = State::Idle;
    bool facingRight = true;
    bool isGrounded = false;
    bool isAttacking = false;

    int health = Config::MAX_HEALTH;
    int soulEnergy = 0;

    float dashTimer = 0, dashCooldown = 0;
    float attackTimer = 0, attackCooldown = 0;
    float invincibility = 0, hurtTimer = 0;
    float coyoteTimer = 0, jumpBufferTimer = 0;
    float flightTimer = 0, invocationTimer = 0;
    float animTimer = 0;

    std::string invocationSeq;
    sf::FloatRect attackBounds;
    PlayerStats stats;

    mutable ParticleSystem trail{1000};
    mutable ParticleSystem dragonFx{500};
};

// ============================================================================
// ENNEMI (VOL LIMITÉ + MARCHE AU SOL)
// ============================================================================

class Enemy {
public:
    enum class Type { Red, Blue, Yellow };
    enum class MovementState { Walking, Flying, Falling };

    Enemy(sf::Vector2f pos, Type type, int waveNumber)
    : position(pos), startPos(pos), type(type) {

        float waveMult = 1.f + waveNumber * 0.15f;
        baseHealth = int(3 * waveMult);
        health = baseHealth;
        damage = 1 + waveNumber / 5;
        speed = 80.f + waveNumber * 5.f;
        flyCooldown = Random::instance().range(2.f, 6.f);

        setupVisuals();
    }

    void setupVisuals() {
        float radius = 20.f;
        sf::Color color;

        switch (type) {
            case Type::Red: color = sf::Color(200, 80, 80, 230); break;
            case Type::Blue: color = sf::Color(80, 120, 200, 230); radius = 22.f; break;
            case Type::Yellow: color = sf::Color(220, 200, 80, 230); radius = 18.f; break;
        }

        body.setRadius(radius);
        body.setOrigin({radius, radius});
        body.setFillColor(color);
        body.setOutlineThickness(2.f);
        body.setOutlineColor(sf::Color(color.r / 2, color.g / 2, color.b / 2, 200));

        eye.setRadius(5.f);
        eye.setOrigin({5.f, 5.f});
        eye.setFillColor(type == Type::Yellow ? sf::Color(50, 50, 50) : sf::Color(255, 200, 50));

        baseColor = color;
    }

    void update(float dt, const sf::Vector2f& playerPos,
                std::vector<Projectile>& projectiles,
                const std::vector<Platform>& platforms) {
        if (!alive) return;

        updateFlightState(dt, playerPos);

        if (moveState != MovementState::Flying) {
            velocity.y += Config::GRAVITY * dt;
            velocity.y = std::min(velocity.y, 600.f);
        }

        switch (moveState) {
            case MovementState::Walking: updateWalking(dt, playerPos, projectiles); break;
            case MovementState::Flying: updateFlying(dt, playerPos, projectiles); break;
            case MovementState::Falling: velocity.x *= 0.98f; break;
        }

        position += velocity * dt;

        isGrounded = false;
        for (const auto& plat : platforms) resolveCollision(plat);

        if (position.y > 1030.f) {
            position.y = 1030.f;
            velocity.y = 0;
            isGrounded = true;
            if (moveState == MovementState::Falling) moveState = MovementState::Walking;
        }

        position.x = std::clamp(position.x, 120.f, 2880.f);

        float visualY = position.y;
        if (isGrounded && moveState == MovementState::Walking) {
            visualY += std::sin(animTimer * 3.f) * 2.f;
        }

        body.setPosition({position.x, visualY});
        eye.setPosition({position.x + (facingRight ? 6.f : -6.f), visualY - 5.f});
        animTimer += dt;

        if (hitFlash > 0) {
            hitFlash -= dt;
            body.setFillColor(sf::Color::White);
        } else {
            body.setFillColor(baseColor);
        }

        particles.update(dt);
                }

                void updateFlightState(float dt, const sf::Vector2f& playerPos) {
                    switch (moveState) {
                        case MovementState::Walking:
                            flyCooldown -= dt;
                            if (flyCooldown <= 0 && isGrounded) {
                                float dist = Math::distance(position, playerPos);
                                if (dist < 500.f && Random::instance().range(0, 100) < 30) {
                                    moveState = MovementState::Flying;
                                    flyTimer = Config::ENEMY_FLY_DURATION;
                                    velocity.y = -200.f;

                                    ParticleConfig cfg;
                                    cfg.startColor = baseColor;
                                    cfg.endColor = sf::Color(baseColor.r/2, baseColor.g/2, baseColor.b/2, 0);
                                    cfg.direction = 1.57f; cfg.spread = 0.8f;
                                    cfg.minSpeed = 50.f; cfg.maxSpeed = 150.f;
                                    cfg.minLife = 0.3f; cfg.maxLife = 0.5f;
                                    particles.emit(position + sf::Vector2f{0, 15.f}, cfg, 15);
                                } else {
                                    flyCooldown = Random::instance().range(3.f, 8.f);
                                }
                            }
                            break;
                        case MovementState::Flying:
                            flyTimer -= dt;
                            if (flyTimer <= 0) {
                                moveState = MovementState::Falling;
                                flyCooldown = Config::ENEMY_FLY_COOLDOWN;
                            }
                            break;
                        case MovementState::Falling: break;
                    }
                }

                void updateWalking(float dt, const sf::Vector2f& playerPos, std::vector<Projectile>& projectiles) {
                    float dist = Math::distance(position, playerPos);

                    if (dist < 400.f) {
                        float dir = playerPos.x > position.x ? 1.f : -1.f;
                        velocity.x = dir * speed;
                        facingRight = dir > 0;
                    } else {
                        velocity.x = speed * patrolDir * 0.5f;
                        if (position.x > startPos.x + patrolRange) patrolDir = -1;
                        else if (position.x < startPos.x - patrolRange) patrolDir = 1;
                        facingRight = patrolDir > 0;
                    }

                    if (type == Type::Blue) {
                        shootCooldown -= dt;
                        if (shootCooldown <= 0 && dist < 500.f) {
                            shootCooldown = 2.5f;
                            sf::Vector2f dir = Math::normalize(playerPos - position);
                            projectiles.emplace_back(position, dir, 200.f, sf::Color(100, 150, 255));

                            ParticleConfig cfg;
                            cfg.startColor = sf::Color(100, 150, 255, 255);
                            cfg.endColor = sf::Color(50, 100, 200, 0);
                            cfg.direction = std::atan2(dir.y, dir.x);
                            cfg.spread = 0.3f;
                            cfg.minSpeed = 100.f; cfg.maxSpeed = 200.f;
                            cfg.minLife = 0.2f; cfg.maxLife = 0.4f;
                            particles.emit(position, cfg, 15);
                        }
                    }

                    if (type == Type::Yellow && isGrounded) {
                        jumpCooldown -= dt;
                        if (jumpCooldown <= 0 && dist < 300.f) {
                            jumpCooldown = 1.5f;
                            velocity.y = -400.f;
                            isGrounded = false;

                            ParticleConfig cfg;
                            cfg.startColor = sf::Color(255, 220, 100, 255);
                            cfg.endColor = sf::Color(200, 150, 50, 0);
                            cfg.direction = 1.57f; cfg.spread = 0.8f;
                            cfg.minSpeed = 80.f; cfg.maxSpeed = 150.f;
                            cfg.minLife = 0.2f; cfg.maxLife = 0.4f;
                            particles.emit(position + sf::Vector2f{0, 15.f}, cfg, 15);
                        }
                    }
                }

                void updateFlying(float dt, const sf::Vector2f& playerPos, std::vector<Projectile>& projectiles) {
                    sf::Vector2f dir = Math::normalize(playerPos - position);
                    velocity.x = Math::lerp(velocity.x, dir.x * speed * 1.5f, dt * 3.f);
                    velocity.y = Math::lerp(velocity.y, dir.y * speed * 0.8f, dt * 2.f);

                    if (position.y < 200.f) velocity.y = std::max(velocity.y, 0.f);

                    facingRight = velocity.x > 0;

                    ParticleConfig cfg;
                    cfg.startColor = sf::Color(baseColor.r, baseColor.g, baseColor.b, 150);
                    cfg.endColor = sf::Color(baseColor.r/2, baseColor.g/2, baseColor.b/2, 0);
                    cfg.direction = 1.57f; cfg.spread = 0.5f;
                    cfg.minSpeed = 30.f; cfg.maxSpeed = 60.f;
                    cfg.minLife = 0.2f; cfg.maxLife = 0.4f;
                    cfg.minSize = 4.f; cfg.maxSize = 8.f;

                    if (int(animTimer * 10) % 3 == 0) {
                        particles.emit(position + sf::Vector2f{0, 10.f}, cfg, 1);
                    }

                    if (type == Type::Blue) {
                        shootCooldown -= dt;
                        float dist = Math::distance(position, playerPos);
                        if (shootCooldown <= 0 && dist < 600.f) {
                            shootCooldown = 1.5f;
                            sf::Vector2f shootDir = Math::normalize(playerPos - position);
                            projectiles.emplace_back(position, shootDir, 250.f, sf::Color(100, 150, 255));
                        }
                    }
                }

                void resolveCollision(const Platform& plat) {
                    sf::FloatRect bounds = {{position.x - 15.f, position.y - 15.f}, {30.f, 30.f}};
                    sf::FloatRect platBounds = plat.getBounds();

                    if (!bounds.findIntersection(platBounds)) return;

                    float oT = (bounds.position.y + bounds.size.y) - platBounds.position.y;

                    if (velocity.y > 0 && oT < 20.f) {
                        position.y = platBounds.position.y - 15.f;
                        velocity.y = 0;
                        isGrounded = true;
                        if (moveState == MovementState::Falling) moveState = MovementState::Walking;
                    }
                }

                void takeDamage(int dmg) {
                    if (!alive) return;
                    health -= dmg;
                    hitFlash = 0.15f;

                    if (health <= 0) {
                        alive = false;
                        ParticleConfig cfg;
                        cfg.startColor = baseColor;
                        cfg.endColor = sf::Color(baseColor.r/2, baseColor.g/2, baseColor.b/2, 0);
                        cfg.minSpeed = 150.f; cfg.maxSpeed = 300.f;
                        cfg.spread = 3.14159f;
                        cfg.minLife = 0.5f; cfg.maxLife = 1.f;
                        particles.emit(position, cfg, 50);
                    }
                }

                void draw(sf::RenderTarget& target) const {
                    particles.draw(target);
                    if (!alive) return;

                    if (type == Type::Blue) {
                        sf::CircleShape glowShape(body.getRadius() * 1.5f);
                        glowShape.setOrigin({glowShape.getRadius(), glowShape.getRadius()});
                        glowShape.setPosition(body.getPosition());
                        glowShape.setFillColor(sf::Color(80, 120, 200, 40));
                        target.draw(glowShape, sf::RenderStates(sf::BlendAdd));
                    }

                    if (moveState == MovementState::Flying) {
                        float wingAnim = std::sin(animTimer * 15.f) * 10.f;

                        sf::ConvexShape wing1, wing2;
                        wing1.setPointCount(3);
                        wing2.setPointCount(3);

                        wing1.setPoint(0, {0, 0});
                        wing1.setPoint(1, {-20.f, -10.f + wingAnim});
                        wing1.setPoint(2, {-15.f, 5.f});

                        wing2.setPoint(0, {0, 0});
                        wing2.setPoint(1, {20.f, -10.f + wingAnim});
                        wing2.setPoint(2, {15.f, 5.f});

                        wing1.setPosition(body.getPosition());
                        wing2.setPosition(body.getPosition());

                        wing1.setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b, 150));
                        wing2.setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b, 150));

                        target.draw(wing1);
                        target.draw(wing2);

                        float flyRatio = flyTimer / Config::ENEMY_FLY_DURATION;
                        sf::RectangleShape flyBar{{30.f * flyRatio, 3.f}};
                        flyBar.setPosition({position.x - 15.f, position.y - 35.f});
                        flyBar.setFillColor(sf::Color(100, 200, 255, 200));
                        target.draw(flyBar);
                    }

                    target.draw(body);
                    target.draw(eye);

                    if (health < baseHealth) {
                        float healthRatio = float(health) / baseHealth;
                        sf::RectangleShape hpBg{{40.f, 5.f}};
                        hpBg.setPosition({position.x - 20.f, position.y - 40.f});
                        hpBg.setFillColor(sf::Color(50, 50, 50, 200));
                        target.draw(hpBg);

                        sf::RectangleShape hpFill{{40.f * healthRatio, 5.f}};
                        hpFill.setPosition({position.x - 20.f, position.y - 40.f});
                        hpFill.setFillColor(sf::Color(220, 80, 80, 220));
                        target.draw(hpFill);
                    }
                }

                sf::FloatRect getBounds() const {
                    float r = body.getRadius();
                    return {{position.x - r, position.y - r}, {r * 2, r * 2}};
                }

                bool isAlive() const { return alive; }
                int getDamage() const { return damage; }

private:
    sf::Vector2f position, startPos;
    sf::Vector2f velocity{0, 0};
    sf::CircleShape body, eye;
    sf::Color baseColor;
    Type type;
    MovementState moveState = MovementState::Walking;

    float patrolRange = 150.f, speed = 80.f;
    int patrolDir = 1;
    bool facingRight = true, isGrounded = true;
    float animTimer = 0, hitFlash = 0;
    float shootCooldown = 2.f, jumpCooldown = 1.f;
    float flyTimer = 0, flyCooldown = 3.f;

    int health, baseHealth, damage = 1;
    bool alive = true;

    mutable ParticleSystem particles{200};
};

// ============================================================================
// WAVE MANAGER
// ============================================================================

class WaveManager {
public:
    void startWave(int wave) {
        currentWave = wave;
        enemiesRemaining = 0;
        waveComplete = false;
        spawnTimer = 0;
        enemiesToSpawn.clear();

        int baseEnemies = 3 + wave * 2;
        int maxEnemies = std::min(baseEnemies, 20);

        for (int i = 0; i < maxEnemies; ++i) {
            Enemy::Type type;
            int roll = Random::instance().range(0, 100);

            if (wave < 3) type = Enemy::Type::Red;
            else if (wave < 5) type = roll < 70 ? Enemy::Type::Red : Enemy::Type::Blue;
            else if (wave < 8) {
                if (roll < 50) type = Enemy::Type::Red;
                else if (roll < 80) type = Enemy::Type::Blue;
                else type = Enemy::Type::Yellow;
            } else {
                if (roll < 35) type = Enemy::Type::Red;
                else if (roll < 65) type = Enemy::Type::Blue;
                else type = Enemy::Type::Yellow;
            }

            enemiesToSpawn.push_back(type);
        }

        enemiesRemaining = static_cast<int>(enemiesToSpawn.size());
    }

    void update(float dt, std::vector<std::unique_ptr<Enemy>>& enemies, const sf::Vector2f& playerPos) {
        if (enemiesToSpawn.empty()) {
            bool allDead = true;
            for (const auto& e : enemies) if (e->isAlive()) { allDead = false; break; }
            if (allDead && enemiesRemaining == 0) waveComplete = true;
            return;
        }

        spawnTimer += dt;
        if (spawnTimer >= spawnInterval) {
            spawnTimer = 0;

            Enemy::Type type = enemiesToSpawn.back();
            enemiesToSpawn.pop_back();

            float spawnX = playerPos.x < 1500.f ?
            Random::instance().range(2000.f, 2800.f) :
            Random::instance().range(200.f, 1000.f);

            enemies.push_back(std::make_unique<Enemy>(
                sf::Vector2f{spawnX, 1000.f}, type, currentWave));

            spawnInterval = std::max(0.3f, 1.5f - currentWave * 0.1f);
        }
    }

    int getCurrentWave() const { return currentWave; }
    int getEnemiesRemaining() const { return enemiesRemaining; }
    bool isWaveComplete() const { return waveComplete; }
    void enemyKilled() { if (enemiesRemaining > 0) enemiesRemaining--; }

private:
    int currentWave = 0, enemiesRemaining = 0;
    bool waveComplete = false;
    float spawnTimer = 0, spawnInterval = 1.f;
    std::vector<Enemy::Type> enemiesToSpawn;
};

// ============================================================================
// UPGRADE SYSTEM
// ============================================================================

class UpgradeSystem {
public:
    struct Upgrade {
        std::string name, description;
        sf::Color color;
        int upgradeType;
    };

    void generateChoices() {
        choices.clear();

        std::vector<Upgrade> allUpgrades = {
            {"Vitesse+", "Deplacement +20%", sf::Color(100, 200, 100), 0},
            {"Saut+", "Hauteur +15%", sf::Color(100, 150, 255), 1},
            {"Attaque+", "Degats +1", sf::Color(255, 100, 100), 2},
            {"Portee+", "Portee +30%", sf::Color(255, 150, 50), 3},
            {"Dash+", "Cooldown -25%", sf::Color(200, 100, 255), 4},
            {"Vol+", "Duree +2s", sf::Color(100, 220, 255), 5},
            {"Vie+", "PV max +1", sf::Color(255, 100, 150), 6},
            {"Vitesse ATK+", "Attaque +25%", sf::Color(255, 200, 100), 7},
        };

        std::vector<int> indices;
        for (size_t i = 0; i < allUpgrades.size(); ++i) indices.push_back(static_cast<int>(i));

        for (int i = 0; i < 3 && !indices.empty(); ++i) {
            int idx = Random::instance().range(0, int(indices.size()) - 1);
            choices.push_back(allUpgrades[indices[idx]]);
            indices.erase(indices.begin() + idx);
        }

        selectedIndex = 0;
        isActive = true;
    }

    int update(const InputManager& input) {
        if (!isActive) return -1;

        if (input.justPressed("left")) selectedIndex = (selectedIndex + choices.size() - 1) % choices.size();
        if (input.justPressed("right")) selectedIndex = (selectedIndex + 1) % choices.size();

        if (input.justPressed("num1") && choices.size() > 0) return 0;
        if (input.justPressed("num2") && choices.size() > 1) return 1;
        if (input.justPressed("num3") && choices.size() > 2) return 2;

        if (input.justPressed("confirm")) return static_cast<int>(selectedIndex);

        return -1;
    }

    void applyUpgrade(int index, PlayerStats& stats) {
        if (index >= 0 && index < int(choices.size())) {
            switch (choices[index].upgradeType) {
                case 0: stats.speedMultiplier += 0.2f; break;
                case 1: stats.jumpMultiplier += 0.15f; break;
                case 2: stats.attackDamage += 1; break;
                case 3: stats.attackRange *= 1.3f; break;
                case 4: stats.dashCooldownMultiplier *= 0.75f; break;
                case 5: stats.flightDurationMultiplier += 0.4f; break;
                case 6: stats.maxHealth += 1; break;
                case 7: stats.attackSpeed += 0.25f; break;
            }
        }
        isActive = false;
    }

    void draw(sf::RenderTarget& target) const {
        if (!isActive) return;

        sf::Vector2f size = target.getView().getSize();
        sf::Vector2f center = target.getView().getCenter();
        float left = center.x - size.x / 2.f;
        float top = center.y - size.y / 2.f;

        sf::RectangleShape overlay;
        overlay.setSize(size);
        overlay.setPosition({left, top});
        overlay.setFillColor(sf::Color(0, 0, 0, 200));
        target.draw(overlay);

        auto& font = FontManager::instance().getFont();
        bool hasFont = FontManager::instance().isLoaded();

        if (hasFont) {
            sf::Text title(font, "AMELIORATION", 48);
            title.setFillColor(sf::Color(150, 200, 255));
            sf::FloatRect tb = title.getGlobalBounds();
            title.setPosition({center.x - tb.size.x / 2.f, top + 80.f});
            target.draw(title);

            sf::Text sub(font, "Choisissez (1, 2, 3 ou Fleches + Entree)", 20);
            sub.setFillColor(sf::Color(150, 150, 150));
            sf::FloatRect sb = sub.getGlobalBounds();
            sub.setPosition({center.x - sb.size.x / 2.f, top + 140.f});
            target.draw(sub);
        }

        float startX = center.x - (choices.size() * 220.f) / 2.f;

        for (size_t i = 0; i < choices.size(); ++i) {
            const auto& upgrade = choices[i];
            float x = startX + i * 220.f;
            float y = center.y - 100.f;
            bool selected = (i == selectedIndex);

            sf::RectangleShape card{{200.f, 280.f}};
            card.setPosition({x, y + (selected ? -15.f : 0.f)});
            card.setFillColor(selected ? sf::Color(50, 60, 80, 250) : sf::Color(30, 40, 55, 230));
            card.setOutlineThickness(selected ? 4.f : 2.f);
            card.setOutlineColor(selected ? upgrade.color : sf::Color(80, 90, 110, 200));
            target.draw(card);

            sf::CircleShape numBg(22.f);
            numBg.setOrigin({22.f, 22.f});
            numBg.setPosition({x + 100.f, y + (selected ? -15.f : 0.f) + 45.f});
            numBg.setFillColor(upgrade.color);
            target.draw(numBg);

            if (hasFont) {
                sf::Text num(font, std::to_string(i + 1), 28);
                num.setFillColor(sf::Color::White);
                num.setStyle(sf::Text::Bold);
                sf::FloatRect nb = num.getGlobalBounds();
                num.setPosition({x + 100.f - nb.size.x / 2.f, y + (selected ? -15.f : 0.f) + 28.f});
                target.draw(num);
            }

            sf::CircleShape icon(35.f);
            icon.setOrigin({35.f, 35.f});
            icon.setPosition({x + 100.f, y + (selected ? -15.f : 0.f) + 120.f});
            icon.setFillColor(upgrade.color);
            target.draw(icon);

            if (hasFont) {
                sf::Text name(font, upgrade.name, 22);
                name.setFillColor(sf::Color::White);
                name.setStyle(sf::Text::Bold);
                sf::FloatRect nameb = name.getGlobalBounds();
                name.setPosition({x + 100.f - nameb.size.x / 2.f, y + (selected ? -15.f : 0.f) + 180.f});
                target.draw(name);

                sf::Text desc(font, upgrade.description, 16);
                desc.setFillColor(sf::Color(180, 180, 180));
                sf::FloatRect descb = desc.getGlobalBounds();
                desc.setPosition({x + 100.f - descb.size.x / 2.f, y + (selected ? -15.f : 0.f) + 220.f});
                target.draw(desc);
            }
        }
    }

    bool active() const { return isActive; }

private:
    std::vector<Upgrade> choices;
    size_t selectedIndex = 0;
    bool isActive = false;
};

// ============================================================================
// HUD
// ============================================================================

class GameHUD {
public:
    void update(int health, int maxHealth, int soul, float flight,
                int wave, int enemiesLeft, const PlayerStats& stats) {
        currentHealth = health;
        currentMaxHealth = maxHealth;
        currentSoul = soul;
        flightTime = flight;
        currentWave = wave;
        currentEnemies = enemiesLeft;
        playerStats = stats;
        pulseTimer += 1.f/60.f;
                }

                void draw(sf::RenderTarget& target) const {
                    sf::Vector2f size = target.getView().getSize();
                    sf::Vector2f center = target.getView().getCenter();
                    float left = center.x - size.x / 2.f;
                    float top = center.y - size.y / 2.f;

                    auto& font = FontManager::instance().getFont();
                    bool hasFont = FontManager::instance().isLoaded();

                    // Conteneur santé
                    sf::RectangleShape healthBox{{260.f, 80.f}};
                    healthBox.setPosition({left + 20.f, top + 20.f});
                    healthBox.setFillColor(sf::Color(10, 12, 20, 220));
                    healthBox.setOutlineThickness(2.f);
                    healthBox.setOutlineColor(sf::Color(60, 70, 90, 200));
                    target.draw(healthBox);

                    if (hasFont) {
                        sf::Text vieLabel(font, "VIE", 14);
                        vieLabel.setFillColor(sf::Color(150, 150, 150));
                        vieLabel.setPosition({left + 30.f, top + 25.f});
                        target.draw(vieLabel);
                    }

                    for (int i = 0; i < currentMaxHealth; ++i) {
                        int col = i % 6;
                        int row = i / 6;
                        sf::CircleShape heart(9.f);
                        heart.setOrigin({9.f, 9.f});
                        heart.setPosition({left + 45.f + col * 32.f, top + 55.f + row * 22.f});

                        if (i < currentHealth) {
                            float pulse = 1.f + std::sin(pulseTimer * 2.f + i * 0.5f) * 0.1f;
                            heart.setScale({pulse, pulse});
                            heart.setFillColor(sf::Color(220, 60, 80, 255));
                        } else {
                            heart.setFillColor(sf::Color(40, 30, 35, 200));
                        }
                        target.draw(heart);
                    }

                    // Barre de vol
                    if (flightTime > 0) {
                        sf::RectangleShape flightBg{{280.f, 22.f}};
                        flightBg.setPosition({center.x - 140.f, top + 20.f});
                        flightBg.setFillColor(sf::Color(15, 20, 30, 220));
                        flightBg.setOutlineThickness(2.f);
                        flightBg.setOutlineColor(sf::Color(80, 150, 220, 200));
                        target.draw(flightBg);

                        float progress = flightTime / (Config::FLIGHT_DURATION * playerStats.flightDurationMultiplier);
                        sf::RectangleShape flightFill{{276.f * progress, 18.f}};
                        flightFill.setPosition({center.x - 138.f, top + 22.f});
                        flightFill.setFillColor(sf::Color(100, 200, 255, 230));
                        target.draw(flightFill);

                        if (hasFont) {
                            sf::Text flightLabel(font, "VOL DRAGON", 12);
                            flightLabel.setFillColor(sf::Color::White);
                            sf::FloatRect fb = flightLabel.getGlobalBounds();
                            flightLabel.setPosition({center.x - fb.size.x / 2.f, top + 23.f});
                            target.draw(flightLabel);
                        }
                    }

                    // Vague
                    sf::RectangleShape waveBox{{180.f, 60.f}};
                    waveBox.setPosition({left + size.x - 200.f, top + 20.f});
                    waveBox.setFillColor(sf::Color(10, 12, 20, 220));
                    waveBox.setOutlineThickness(2.f);
                    waveBox.setOutlineColor(sf::Color(60, 70, 90, 200));
                    target.draw(waveBox);

                    if (hasFont) {
                        sf::Text waveLabel(font, "VAGUE " + std::to_string(currentWave), 22);
                        waveLabel.setFillColor(sf::Color(255, 220, 100));
                        waveLabel.setStyle(sf::Text::Bold);
                        waveLabel.setPosition({left + size.x - 185.f, top + 28.f});
                        target.draw(waveLabel);

                        sf::Text enemyLabel(font, "Ennemis: " + std::to_string(currentEnemies), 14);
                        enemyLabel.setFillColor(sf::Color(255, 100, 100));
                        enemyLabel.setPosition({left + size.x - 185.f, top + 55.f});
                        target.draw(enemyLabel);
                    }
                }

private:
    int currentHealth = Config::MAX_HEALTH;
    int currentMaxHealth = Config::MAX_HEALTH;
    int currentSoul = 0;
    float flightTime = 0;
    int currentWave = 1;
    int currentEnemies = 0;
    PlayerStats playerStats;
    mutable float pulseTimer = 0;
};

// ============================================================================
// CONTROLS HINT
// ============================================================================

class ControlsHint {
public:
    void draw(sf::RenderTarget& target) const {
        sf::Vector2f size = target.getView().getSize();
        sf::Vector2f center = target.getView().getCenter();
        float left = center.x - size.x / 2.f;
        float bottom = center.y + size.y / 2.f;

        sf::RectangleShape bg{{size.x - 40.f, 40.f}};
        bg.setPosition({left + 20.f, bottom - 55.f});
        bg.setFillColor(sf::Color(10, 12, 20, 180));
        bg.setOutlineThickness(1.f);
        bg.setOutlineColor(sf::Color(50, 60, 80, 150));
        target.draw(bg);

        if (FontManager::instance().isLoaded()) {
            auto& font = FontManager::instance().getFont();
            sf::Text controls(font, "[Fleches/WASD] Deplacer   [Espace] Sauter   [V] Attaquer   [Shift] Dash   [Y-H-P] Dragon   [Echap] Pause", 14);
            controls.setFillColor(sf::Color(150, 150, 150));
            sf::FloatRect cb = controls.getGlobalBounds();
            controls.setPosition({center.x - cb.size.x / 2.f, bottom - 48.f});
            target.draw(controls);
        }
    }
};

// ============================================================================
// PAUSE MENU (CENTRÉ)
// ============================================================================

class PauseMenu {
public:
    enum class Result { None, Resume, MainMenu, Quit };

    Result update(const InputManager& input) {
        if (input.justPressed("down")) selected = (selected + 1) % 3;
        if (input.justPressed("up")) selected = (selected + 2) % 3;

        if (input.justPressed("pause")) return Result::Resume;

        if (input.justPressed("confirm")) {
            if (selected == 0) return Result::Resume;
            if (selected == 1) return Result::MainMenu;
            if (selected == 2) return Result::Quit;
        }

        return Result::None;
    }

    void draw(sf::RenderTarget& target) const {
        sf::Vector2f size = target.getView().getSize();
        sf::Vector2f center = target.getView().getCenter();
        float left = center.x - size.x / 2.f;
        float top = center.y - size.y / 2.f;

        sf::RectangleShape overlay;
        overlay.setSize(size);
        overlay.setPosition({left, top});
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        target.draw(overlay);

        sf::RectangleShape panel{{380.f, 320.f}};
        panel.setPosition({center.x - 190.f, center.y - 160.f});
        panel.setFillColor(sf::Color(15, 20, 35, 250));
        panel.setOutlineThickness(3.f);
        panel.setOutlineColor(sf::Color(100, 150, 200, 200));
        target.draw(panel);

        auto& font = FontManager::instance().getFont();
        bool hasFont = FontManager::instance().isLoaded();

        if (hasFont) {
            sf::Text title(font, "PAUSE", 42);
            title.setFillColor(sf::Color(150, 200, 255));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect tb = title.getGlobalBounds();
            title.setPosition({center.x - tb.size.x / 2.f, center.y - 140.f});
            target.draw(title);
        }

        std::vector<std::string> options = {"Reprendre", "Menu Principal", "Quitter"};

        for (size_t i = 0; i < options.size(); ++i) {
            float btnY = center.y - 50.f + i * 65.f;

            sf::RectangleShape btn{{280.f, 50.f}};
            btn.setPosition({center.x - 140.f, btnY});

            if (i == selected) {
                btn.setFillColor(sf::Color(60, 80, 120, 240));
                btn.setOutlineThickness(2.f);
                btn.setOutlineColor(sf::Color(150, 200, 255, 255));
            } else {
                btn.setFillColor(sf::Color(40, 50, 70, 200));
                btn.setOutlineThickness(1.f);
                btn.setOutlineColor(sf::Color(80, 100, 130, 150));
            }
            target.draw(btn);

            if (hasFont) {
                sf::Text opt(font, options[i], 22);
                opt.setFillColor(sf::Color::White);
                sf::FloatRect ob = opt.getGlobalBounds();
                opt.setPosition({center.x - ob.size.x / 2.f, btnY + 12.f});
                target.draw(opt);
            }
        }
    }

    void reset() { selected = 0; }

private:
    size_t selected = 0;
};

// ============================================================================
// MENU PRINCIPAL (CENTRÉ DYNAMIQUEMENT)
// ============================================================================

class MainMenu {
public:
    enum class Result { None, Play, Quit };

    MainMenu() {
        particles.gravity = -15.f;
        particles.drag = 0.2f;
    }

    Result update(float dt, const InputManager& input) {
        timer += dt;

        if (input.justPressed("down") || input.justPressed("up")) {
            selected = (selected + 1) % 2;
        }

        if (input.justPressed("confirm")) {
            if (selected == 0) return Result::Play;
            if (selected == 1) return Result::Quit;
        }

        return Result::None;
    }

    void draw(sf::RenderTarget& target) {
        // Obtenir les dimensions réelles de l'écran
        sf::Vector2f viewSize = target.getView().getSize();
        sf::Vector2f viewCenter = target.getView().getCenter();

        float left = viewCenter.x - viewSize.x / 2.f;
        float top = viewCenter.y - viewSize.y / 2.f;
        float centerX = viewCenter.x;
        float centerY = viewCenter.y;
        float width = viewSize.x;
        float height = viewSize.y;

        // Background plein écran
        sf::RectangleShape bg;
        bg.setSize(viewSize);
        bg.setPosition({left, top});
        bg.setFillColor(sf::Color(8, 10, 20));
        target.draw(bg);

        // Particules
        if (Random::instance().range(0, 100) < 8) {
            ParticleConfig cfg;
            cfg.startColor = sf::Color(80, 130, 200, 120);
            cfg.endColor = sf::Color(50, 80, 150, 0);
            cfg.minSpeed = 15.f; cfg.maxSpeed = 40.f;
            cfg.direction = -1.57f; cfg.spread = 0.4f;
            cfg.minLife = 4.f; cfg.maxLife = 7.f;
            particles.emit({left + Random::instance().range(0.f, width), top + height + 20.f}, cfg, 1);
        }
        particles.update(1.f/60.f);
        particles.draw(target);

        // Logo centré
        float pulse = std::sin(timer * 2.f) * 0.1f + 1.f;

        sf::CircleShape logoGlow(130.f);
        logoGlow.setOrigin({130.f, 130.f});
        logoGlow.setPosition({centerX, centerY - 150.f});
        logoGlow.setFillColor(sf::Color(80, 150, 255, 40));
        logoGlow.setScale({pulse, pulse});
        target.draw(logoGlow, sf::RenderStates(sf::BlendAdd));

        sf::CircleShape logo(100.f);
        logo.setOrigin({100.f, 100.f});
        logo.setPosition({centerX, centerY - 150.f});
        logo.setFillColor(sf::Color(100, 180, 255, 220));
        logo.setOutlineThickness(5.f);
        logo.setOutlineColor(sf::Color(150, 200, 255, 255));
        target.draw(logo);

        auto& font = FontManager::instance().getFont();
        bool hasFont = FontManager::instance().isLoaded();

        if (hasFont) {
            // Titre centré
            sf::Text title(font, "SOUL WORLD", 72);
            title.setFillColor(sf::Color(150, 200, 255));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect titleBounds = title.getGlobalBounds();
            title.setPosition({centerX - titleBounds.size.x / 2.f, centerY - 20.f});
            target.draw(title);

            // Sous-titre
            sf::Text subtitle(font, "Infinite Waves", 28);
            subtitle.setFillColor(sf::Color(200, 180, 100));
            sf::FloatRect subBounds = subtitle.getGlobalBounds();
            subtitle.setPosition({centerX - subBounds.size.x / 2.f, centerY + 60.f});
            target.draw(subtitle);
        }

        // Boutons centrés
        std::vector<std::string> options = {"JOUER", "QUITTER"};
        float buttonY = centerY + 140.f;

        for (size_t i = 0; i < options.size(); ++i) {
            float btnX = centerX - 140.f;
            float btnY = buttonY + i * 75.f;

            sf::RectangleShape btn{{280.f, 55.f}};
            btn.setPosition({btnX, btnY});

            if (i == selected) {
                btn.setFillColor(sf::Color(60, 80, 120, 240));
                btn.setOutlineThickness(3.f);
                btn.setOutlineColor(sf::Color(150, 200, 255, 255));

                sf::CircleShape arrow(12.f, 3);
                arrow.setOrigin({12.f, 12.f});
                arrow.setRotation(sf::degrees(90.f));
                arrow.setPosition({btnX - 30.f, btnY + 27.f});
                arrow.setFillColor(sf::Color(150, 200, 255, 255));
                target.draw(arrow);
            } else {
                btn.setFillColor(sf::Color(40, 50, 70, 200));
                btn.setOutlineThickness(2.f);
                btn.setOutlineColor(sf::Color(80, 100, 130, 150));
            }
            target.draw(btn);

            if (hasFont) {
                sf::Text opt(font, options[i], 28);
                opt.setFillColor(sf::Color::White);
                opt.setStyle(sf::Text::Bold);
                sf::FloatRect optBounds = opt.getGlobalBounds();
                opt.setPosition({centerX - optBounds.size.x / 2.f, btnY + 10.f});
                target.draw(opt);
            }
        }

        // Instructions en bas
        if (hasFont) {
            sf::Text hint(font, "Fleches + Entree pour naviguer", 18);
            hint.setFillColor(sf::Color(120, 120, 120));
            sf::FloatRect hintBounds = hint.getGlobalBounds();
            hint.setPosition({centerX - hintBounds.size.x / 2.f, top + height - 80.f});
            target.draw(hint);

            sf::Text version(font, "v1.0 - F11 plein ecran", 14);
            version.setFillColor(sf::Color(80, 80, 80));
            sf::FloatRect verBounds = version.getGlobalBounds();
            version.setPosition({centerX - verBounds.size.x / 2.f, top + height - 45.f});
            target.draw(version);
        }
    }

private:
    size_t selected = 0;
    float timer = 0;
    mutable ParticleSystem particles{300};
};

// ============================================================================
// WAVE COMPLETE POPUP
// ============================================================================

class WaveCompletePopup {
public:
    void show(int wave) { currentWave = wave; timer = 0; isActive = true; }

    void update(float dt) {
        if (!isActive) return;
        timer += dt;
        if (timer >= duration) isActive = false;
    }

    void draw(sf::RenderTarget& target) const {
        if (!isActive) return;

        sf::Vector2f center = target.getView().getCenter();

        float alpha = 1.f;
        if (timer < 0.3f) alpha = timer / 0.3f;
        else if (timer > duration - 0.5f) alpha = (duration - timer) / 0.5f;

        sf::RectangleShape banner{{450.f, 90.f}};
        banner.setPosition({center.x - 225.f, center.y - 45.f});
        banner.setFillColor(sf::Color(20, 40, 60, uint8_t(230 * alpha)));
        banner.setOutlineThickness(3.f);
        banner.setOutlineColor(sf::Color(100, 200, 100, uint8_t(255 * alpha)));
        target.draw(banner);

        if (FontManager::instance().isLoaded()) {
            auto& font = FontManager::instance().getFont();
            sf::Text text(font, "VAGUE " + std::to_string(currentWave) + " COMPLETE!", 36);
            text.setFillColor(sf::Color(100, 255, 100, uint8_t(255 * alpha)));
            text.setStyle(sf::Text::Bold);
            sf::FloatRect tb = text.getGlobalBounds();
            text.setPosition({center.x - tb.size.x / 2.f, center.y - 22.f});
            target.draw(text);
        }
    }

    bool active() const { return isActive; }

private:
    int currentWave = 0;
    float timer = 0, duration = 2.f;
    bool isActive = false;
};

// ============================================================================
// BACKGROUND
// ============================================================================

class Background {
public:
    Background() {
        for (int l = 0; l < 3; ++l) {
            float parallax = 0.1f + l * 0.2f;
            sf::Color color(20 + l * 10, 25 + l * 10, 40 + l * 15);

            for (int i = 0; i < 5 + l * 3; ++i) {
                sf::RectangleShape elem;
                float w = Random::instance().range(150.f, 400.f - l * 100.f);
                float h = Random::instance().range(100.f, 300.f - l * 50.f);
                elem.setSize({w, h});
                elem.setPosition({Random::instance().range(-100.f, 3100.f), 1100.f - h - Random::instance().range(0.f, 100.f)});
                elem.setFillColor(color);
                layers.push_back({elem, parallax});
            }
        }

        particles.gravity = -20.f;
        particles.drag = 0.1f;
    }

    void update(float dt) {
        if (Random::instance().range(0, 100) < 5) {
            ParticleConfig cfg;
            cfg.startColor = sf::Color(100, 120, 180, 100);
            cfg.endColor = sf::Color(80, 100, 150, 0);
            cfg.minSpeed = 10.f; cfg.maxSpeed = 30.f;
            cfg.direction = -1.57f; cfg.spread = 0.5f;
            cfg.minLife = 3.f; cfg.maxLife = 6.f;
            cfg.minSize = 2.f; cfg.maxSize = 4.f;
            particles.emit({Random::instance().range(0.f, 3000.f), 1150.f}, cfg, 1);
        }
        particles.update(dt);
    }

    void draw(sf::RenderTarget& target, sf::Vector2f camOffset) const {
        for (const auto& [elem, parallax] : layers) {
            sf::RectangleShape drawShape = elem;
            drawShape.setPosition(elem.getPosition() - camOffset * parallax);
            target.draw(drawShape);
        }
        particles.draw(target);
    }

private:
    std::vector<std::pair<sf::RectangleShape, float>> layers;
    mutable ParticleSystem particles{500};
};

// ============================================================================
// CAMERA & SCREEN SHAKE
// ============================================================================

class Camera {
public:
    Camera(sf::Vector2f size) : viewSize(size) {
        view.setSize(size);
        view.setCenter(size / 2.f);
    }

    void follow(sf::Vector2f target, float dt) {
        sf::Vector2f desired = target;
        desired.x = std::clamp(desired.x, viewSize.x / 2.f, levelBounds.x - viewSize.x / 2.f);
        desired.y = std::clamp(desired.y, viewSize.y / 2.f, levelBounds.y - viewSize.y / 2.f);
        view.setCenter(Math::lerp(view.getCenter(), desired, 5.f * dt));
    }

    void applyShake(sf::Vector2f offset) { view.setCenter(view.getCenter() + offset); }
    void setLevelBounds(sf::Vector2f bounds) { levelBounds = bounds; }
    sf::View& getView() { return view; }
    sf::Vector2f getCenter() const { return view.getCenter(); }

private:
    sf::View view;
    sf::Vector2f viewSize;
    sf::Vector2f levelBounds{3000.f, 1200.f};
};

class ScreenShake {
public:
    void shake(float intensity, float dur) {
        this->intensity = std::max(this->intensity, intensity);
        duration = std::max(duration, dur);
        timer = 0;
    }

    sf::Vector2f update(float dt) {
        if (duration <= 0) return {0, 0};
        timer += dt;
        if (timer >= duration) { duration = intensity = 0; return {0, 0}; }
        return Random::instance().insideCircle(intensity * (1.f - timer / duration));
    }

private:
    float intensity = 0, duration = 0, timer = 0;
};

// ============================================================================
// GAME
// ============================================================================

enum class GameState { MainMenu, Playing, Paused, Upgrading, GameOver };

class Game {
public:
    Game() : window(sf::VideoMode({Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT}),
                    "Soul World", sf::Style::Default, sf::State::Fullscreen),
                    camera({float(Config::WINDOW_WIDTH), float(Config::WINDOW_HEIGHT)}),
                    player({200.f, 900.f}) {
                        window.setFramerateLimit(60);
                        FontManager::instance().loadFont();
                        camera.setLevelBounds({3000.f, 1200.f});
                        createLevel();
                    }

                    void createLevel() {
                        platforms.clear();
                        platforms.emplace_back(sf::Vector2f{2800.f, 50.f}, sf::Vector2f{100.f, 1050.f});
                        platforms.emplace_back(sf::Vector2f{250.f, 25.f}, sf::Vector2f{300.f, 850.f}, true);
                        platforms.emplace_back(sf::Vector2f{300.f, 25.f}, sf::Vector2f{650.f, 700.f}, true);
                        platforms.emplace_back(sf::Vector2f{200.f, 25.f}, sf::Vector2f{1050.f, 600.f}, true);
                        platforms.emplace_back(sf::Vector2f{350.f, 25.f}, sf::Vector2f{1400.f, 750.f}, true);
                        platforms.emplace_back(sf::Vector2f{200.f, 25.f}, sf::Vector2f{1850.f, 550.f}, true);
                        platforms.emplace_back(sf::Vector2f{300.f, 25.f}, sf::Vector2f{2200.f, 680.f}, true);
                        platforms.emplace_back(sf::Vector2f{50.f, 400.f}, sf::Vector2f{50.f, 700.f});
                        platforms.emplace_back(sf::Vector2f{50.f, 400.f}, sf::Vector2f{2900.f, 700.f});

                        Platform& m1 = platforms.emplace_back(sf::Vector2f{150.f, 25.f}, sf::Vector2f{500.f, 500.f}, true);
                        m1.setMoving(true, 150.f, 1.5f);
                        Platform& m2 = platforms.emplace_back(sf::Vector2f{150.f, 25.f}, sf::Vector2f{1200.f, 450.f}, true);
                        m2.setMoving(false, 100.f, 2.f);
                        Platform& m3 = platforms.emplace_back(sf::Vector2f{200.f, 25.f}, sf::Vector2f{2000.f, 400.f}, true);
                        m3.setMoving(true, 200.f, 1.f);
                    }

                    void startNewGame() {
                        player.fullReset({200.f, 900.f});
                        enemies.clear();
                        projectiles.clear();
                        waveManager.startWave(1);
                        state = GameState::Playing;
                    }

                    void run() {
                        sf::Clock clock;
                        while (window.isOpen()) {
                            float dt = std::min(clock.restart().asSeconds(), 1.f/30.f);
                            handleEvents();
                            update(dt);
                            render();
                        }
                    }

                    void handleEvents() {
                        while (const std::optional event = window.pollEvent()) {
                            if (event->is<sf::Event::Closed>()) window.close();
                            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                                if (key->code == sf::Keyboard::Key::F11) toggleFullscreen();
                            }
                        }
                        input.update();
                    }

                    void toggleFullscreen() {
                        isFullscreen = !isFullscreen;
                        if (isFullscreen) {
                            window.create(sf::VideoMode({Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT}),
                                          "Soul World", sf::Style::Default, sf::State::Fullscreen);
                        } else {
                            window.create(sf::VideoMode({1280, 720}),
                                          "Soul World", sf::Style::Default, sf::State::Windowed);
                        }
                        window.setFramerateLimit(60);
                    }

                    void update(float dt) {
                        switch (state) {
                            case GameState::MainMenu: {
                                auto result = mainMenu.update(dt, input);
                                if (result == MainMenu::Result::Play) startNewGame();
                                else if (result == MainMenu::Result::Quit) window.close();
                                break;
                            }

                            case GameState::Playing: {
                                if (input.justPressed("pause")) {
                                    state = GameState::Paused;
                                    pauseMenu.reset();
                                    break;
                                }

                                for (auto& plat : platforms) plat.update(dt);
                                player.handleInput(input, dt);
                                player.update(dt, platforms);

                                for (auto& proj : projectiles) {
                                    proj.update(dt);
                                    if (proj.isActive() && proj.getBounds().findIntersection(player.getCollisionBounds())) {
                                        player.takeDamage(int(proj.getDamage()));
                                        proj.deactivate();
                                        screenShake.shake(10.f, 0.2f);
                                    }
                                }
                                projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                                                 [](const Projectile& p) { return !p.isActive(); }), projectiles.end());

                                waveManager.update(dt, enemies, player.getPosition());

                                for (auto& enemy : enemies) {
                                    enemy->update(dt, player.getPosition(), projectiles, platforms);

                                    if (player.getIsAttacking() && enemy->isAlive() &&
                                        player.getAttackBounds().findIntersection(enemy->getBounds())) {
                                        enemy->takeDamage(player.getAttackDamage());
                                    screenShake.shake(6.f, 0.1f);
                                    player.addSoul(10);
                                    if (!enemy->isAlive()) { waveManager.enemyKilled(); player.addSoul(20); }
                                        }

                                        if (enemy->isAlive() && player.getCollisionBounds().findIntersection(enemy->getBounds())) {
                                            player.takeDamage(enemy->getDamage());
                                            screenShake.shake(12.f, 0.25f);
                                        }
                                }

                                enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                                                             [](const std::unique_ptr<Enemy>& e) { return !e->isAlive(); }), enemies.end());

                                if (waveManager.isWaveComplete()) {
                                    wavePopup.show(waveManager.getCurrentWave());
                                    upgradeSystem.generateChoices();
                                    state = GameState::Upgrading;
                                }

                                wavePopup.update(dt);
                                camera.follow(player.getPosition(), dt);
                                camera.applyShake(screenShake.update(dt));
                                background.update(dt);

                                hud.update(player.getHealth(), player.getStats().maxHealth,
                                           player.getSoulEnergy(), player.getFlightTimer(),
                                           waveManager.getCurrentWave(), waveManager.getEnemiesRemaining(),
                                           player.getStats());

                                if (player.getState() == Player::State::Dead) {
                                    state = GameState::GameOver;
                                    gameOverTimer = 0;
                                }
                                break;
                            }

                            case GameState::Paused: {
                                auto result = pauseMenu.update(input);
                                if (result == PauseMenu::Result::Resume) state = GameState::Playing;
                                else if (result == PauseMenu::Result::MainMenu) state = GameState::MainMenu;
                                else if (result == PauseMenu::Result::Quit) window.close();
                                break;
                            }

                            case GameState::Upgrading: {
                                wavePopup.update(dt);
                                int choice = upgradeSystem.update(input);
                                if (choice >= 0) {
                                    upgradeSystem.applyUpgrade(choice, player.getStats());
                                    player.heal(1);
                                    waveManager.startWave(waveManager.getCurrentWave() + 1);
                                    state = GameState::Playing;
                                }
                                break;
                            }

                            case GameState::GameOver: {
                                gameOverTimer += dt;
                                if (input.justPressed("confirm") && gameOverTimer > 1.f) startNewGame();
                                if (input.justPressed("back")) state = GameState::MainMenu;
                                break;
                            }
                        }
                    }

                    void render() {
                        window.clear(sf::Color(5, 8, 15));

                        if (state == GameState::MainMenu) {
                            window.setView(window.getDefaultView());
                            mainMenu.draw(window);
                        } else {
                            window.setView(camera.getView());

                            sf::View defView = window.getDefaultView();
                            window.setView(defView);
                            background.draw(window, camera.getCenter() - sf::Vector2f{
                                defView.getSize().x / 2.f, defView.getSize().y / 2.f});
                            window.setView(camera.getView());

                            for (const auto& plat : platforms) plat.draw(window);
                            for (const auto& proj : projectiles) proj.draw(window);
                            for (const auto& enemy : enemies) enemy->draw(window);
                            player.draw(window);

                            window.setView(defView);
                            hud.draw(window);
                            controls.draw(window);
                            wavePopup.draw(window);

                            if (state == GameState::Paused) pauseMenu.draw(window);
                            if (state == GameState::Upgrading) upgradeSystem.draw(window);
                            if (state == GameState::GameOver) drawGameOver();
                        }

                        window.display();
                    }

                    void drawGameOver() {
                        sf::Vector2f center = window.getView().getCenter();
                        sf::Vector2f size = window.getView().getSize();
                        float left = center.x - size.x / 2.f;
                        float top = center.y - size.y / 2.f;

                        float alpha = std::min(gameOverTimer * 150.f, 220.f);
                        sf::RectangleShape overlay;
                        overlay.setSize(size);
                        overlay.setPosition({left, top});
                        overlay.setFillColor(sf::Color(10, 0, 0, uint8_t(alpha)));
                        window.draw(overlay);

                        sf::RectangleShape box{{480.f, 220.f}};
                        box.setPosition({center.x - 240.f, center.y - 110.f});
                        box.setFillColor(sf::Color(30, 15, 20, 250));
                        box.setOutlineThickness(3.f);
                        box.setOutlineColor(sf::Color(200, 80, 80, 255));
                        window.draw(box);

                        if (FontManager::instance().isLoaded()) {
                            auto& font = FontManager::instance().getFont();

                            sf::Text gameOver(font, "GAME OVER", 52);
                            gameOver.setFillColor(sf::Color(255, 100, 100));
                            gameOver.setStyle(sf::Text::Bold);
                            sf::FloatRect gob = gameOver.getGlobalBounds();
                            gameOver.setPosition({center.x - gob.size.x / 2.f, center.y - 90.f});
                            window.draw(gameOver);

                            sf::Text waveReached(font, "Vague atteinte: " + std::to_string(waveManager.getCurrentWave()), 26);
                            waveReached.setFillColor(sf::Color(255, 220, 100));
                            sf::FloatRect wrb = waveReached.getGlobalBounds();
                            waveReached.setPosition({center.x - wrb.size.x / 2.f, center.y - 15.f});
                            window.draw(waveReached);

                            if (gameOverTimer > 1.f) {
                                sf::Text retry(font, "[Entree] Rejouer", 20);
                                retry.setFillColor(sf::Color(100, 200, 100));
                                retry.setPosition({center.x - 170.f, center.y + 45.f});
                                window.draw(retry);

                                sf::Text menu(font, "[Echap] Menu", 20);
                                menu.setFillColor(sf::Color(200, 100, 100));
                                menu.setPosition({center.x + 20.f, center.y + 45.f});
                                window.draw(menu);
                            }
                        }
                    }

private:
    sf::RenderWindow window;
    bool isFullscreen = true;

    GameState state = GameState::MainMenu;
    InputManager input;

    MainMenu mainMenu;
    PauseMenu pauseMenu;
    UpgradeSystem upgradeSystem;
    GameHUD hud;
    ControlsHint controls;
    WaveCompletePopup wavePopup;

    Camera camera;
    ScreenShake screenShake;
    Background background;

    Player player;
    std::vector<Platform> platforms;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<Projectile> projectiles;

    WaveManager waveManager;
    float gameOverTimer = 0;
};

// ============================================================================
// MAIN
// ============================================================================

int main() {
    try {
        Game game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
